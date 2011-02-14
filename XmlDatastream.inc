<?php

// $Id$

/**
 * Repersents a Fedora based XML datastream, extends DOMDocument.
 *
 * This class should never be instantiated. It is meant to provide most of the
 * common functionality that all XML based datastreams share. This class and 
 * its decendants are used to generate XML documents from submitted Drupal forms.
 * Specifically the forms that this class processes must be created by the
 * IngestFormBuilder/EditFormBuilders classes or one of thier decendants.
 */
class XmlDatastreamFormDocument extends DOMDocument {

  /**
   * The drupal form the user has submitted.
   * @var array
   */
  protected $form;
  /**
   * The drupal form_state for the form the user has submitted.
   * @var array
   */
  protected $formState;
  /**
   * The drupal form_state storage index where form build persistant data is kept.
   * form_state['storage']['form_builder']
   * @var array
   */
  protected $storage;
  /**
   * A map of prefixes and namespaces. Where the index is the prefix, and the value is a namespace uri.
   * @var string[]
   */
  protected $nameSpaces;
  /**
   * A map of xpaths and values. Where the index is the xpath, and the value is the node to create.
   * @var string[]
   */
  protected $paths;

  /**
   * Create a XmlDatastreamForm.
   *
   * @param array $form
   *   Drupal form.
   * @param array $form_state
   *   Drupal form_state.
   * @param array $namespaces_to_register
   *   Namespaces needed to be registerd for xpath to work correctly.
   */
  function __construct(&$form, &$form_state, $namespaces_to_register = NULL) {
    parent::__construct(); // DomDocument
    $this->initMembers($form, $form_state, $namespaces_to_register);
    $this->createDocument();
  }

  /**
   * Initialize required instance members.
   *
   * @param array $form
   *   Drupal form.
   * @param array $form_state
   *   Drupal form_state.
   * @param array $namespaces_to_register
   *   Namespaces needed to be registerd for xpath to work correctly.
   */
  private function initMembers(&$form, &$form_state, &$namespaces_to_register) {
    $this->form = &$form;
    $this->formState = &$form_state;
    $this->storage = &$form_state['storage']['form_builder']; // Storage specific to form builder.
    $this->formValues = &$form_state['values']['form_builder'];
    $this->form = &$form['form_builder'];
    $this->formatOutput = TRUE;
    $this->xpath = new DOMXPath($this);
    $this->nameSpaces = $namespaces_to_register;
    foreach ($this->nameSpaces as $prefix => $uri) {
      $this->xpath->registerNamespace($prefix, $uri);
    }
  }

  /**
   * Create all the child nodes of this document.
   *
   * Using submitted form values either modify an existing document, or create a new document from
   * the form values alone.
   */
  private function createDocument() {
    if ($this->shouldModifyExistingDocument()) { // Edit forms.
      $document = $this->getExistingDocument();
      $this->modifyExistingDocument($document);
      $this->loadXML($document->saveXML()); // Is importing quicker?
    }
    else {
      $this->createDocumentFromForm($this->form, $this->formValues, $this);
    }
  }

  /**
   * Should we modify an existing document?
   *
   * @return boolean
   *   TRUE if a document exists and we should modify it.
   */
  private function shouldModifyExistingDocument() {
    return $this->getExistingDocument() != FALSE;
  }

  /**
   * Gets a xml document from persitant storage if it exists.
   *
   * @return DOMDocument
   *   Returns a DOMDocument from persistant storage if it exists. Otherwise it returns FALSE.
   */
  private function getExistingDocument() {
    $document = $this->storage['xml'];
    $exists = isset($document);
    $is_dom_document = get_class($document) == 'DOMDocument';
    return ($exists && $is_dom_document) ? $document : FALSE;
  }

  /**
   * Apply changes to an existing document, using submitted form values and any relevant stored data.
   *
   * @param DOMDocument $document
   *   Document to modify.
   */
  private function modifyExistingDocument($document) {
    
  }

  /**
   * Creates this document from submitted form values and any relevant stored data.
   *
   * @param array $form
   * @param array $form_values
   * @param DOMNode $parent_node
   */
  private function createDocumentFromForm(&$form, &$form_values, $parent_node) {
    foreach ($form as $index => $form_element) { // This loop may change based on values ...
      if (is_numeric($index)) {
        $form_value = $form_values[$index];
        if ($this->canFormElementBeProcessed($form, $form_value)) {
          $node = $this->processFormElementAndValue($form_element, $form_value, $parent_node);
          if (isset($form_element['content'])) {
            $this->createDocumentFromForm($form_element['content'], $value['content'], $node);
          }
        }
      }
    }
  }

  /**
   * Determins if a form element and value pair can be processed.
   *
   * @param array $form
   * @param array $form_value
   * @return boolean
   *   TRUE if this the element form, can be processed, FALSE otherwise.
   */
  private function canFormElementBeProcessed(&$form, &$form_value) {
    $properties = &$this->getFormBuilderProperties($form);
    $value_is_valid_string = is_string($form_value) && (!$properties['#ignore_empty_values']) ? TRUE : $form_value !== '';
    $value_is_array = is_array($form_value);
    $has_xpath = isset($properties['#xpath']);

    $form_value = trim($form_value);
    $value_is_valid = is_string($form_value) && (!$properties['#ignore_empty_values']) ? TRUE : $form_value !== '';
    return $has_xpath && $value_is_valid;
  }

  /**
   * Get properties only relevent to form builder.
   *
   * @param array $form
   *   Drupal form element;
   * @return array
   *   Returns an array of Form Builder properties, if found otherwise FALSE is returned.
   */
  private function getFormBuilderProperties(&$form) {
    return is_array($form['#form_builder']) ? $form['#form_builder'] : FALSE;
  }

  /**
   * Can create more than one DOMNode form certian element types such as list, but most others
   * Only create one DOMNode.
   * 
   * @param array $form
   * @param array $form_value
   * @param DOMNode $parent_node
   * @return DOMNode
   */
  private function processFormElementAndValue(&$form, &$form_value, $parent_node) {
    $properties = &$this->getFormBuilderProperties($form);
    list($path, $is_full_path) = $properties['#xpath'];
    list($parent_path, $force_create) = $properties['#parent_xpath'];
    $xml = $properties['#xml'];

    // Get proper value or values
    $value = $this->getTypeDependantValue($form['#type'], $form_value);
    $single_value = is_string($value) && $value != '';
    $multiple_values = is_array($value);
    $values = $multiple_values ? $value : NULL;

    // Multiple values correspond to a single path per value. Exclusive to lists.
    if ($multiple_values) {
      $count = count($values);
      for ($i = 0; $i < $count; $i++) { // This is exclusively for lists...
        $nodes = &$this->modifyOrCreateNodes("$path[$i]", $is_full_path, $parent_node, $parent_path, $force_create, $values[$i]);
      }
    }
    else {
      $nodes = &$this->modifyOrCreateNodes($path, $is_full_path, $parent_node, $parent_path, $force_create, $value);
    }
  }

  /**
   * Should eventually be removed and some preprocessor handles this sorta junk...
   * @param <type> $type
   * @param <type> $value
   * @return <type>
   */
  private function getTypeDependantValue($type, $value) {
    switch ($type) {
      case 'list':
        return split(';', $value);
      default:
        return $value;
    }
  }

  /**
   * Should break up this function a bit...
   *
   * @param string $path
   * @param boolean $is_full_path
   * @param DOMNode $parent_node
   * @param string $parent_path
   * @param boolean $force_create
   * @param string $value
   * @return DOMNode[]
   */
  private function modifyOrCreateNodes($path, $is_full_path, $parent_node, $parent_path, $force_create, $value) {
    $result = ($is_full_path) ? $this->xpath->query($path) : $this->xpath->query($path, $parent_node);
    if ($result === FALSE) {
      // Display warning and exit.
    }
    else if ($result->length != 0) { // Found, For each found element set its value.
      $modified_nodes = array();
      for ($i = 0; $i < $result->length; $i++) {
        $node = $result->item($i);
        $modified_nodes[] = $this->modifyNode($node, $value);
      }
      return $modified_nodes;
    }
    else { // Not found.
      $result = $this->xpath->query($parent_path);
      if ($result === FALSE) {
        // display a warning, or error.
        return; // Discontiue processing this element, should probably use exceptions and not render the form ... TODO
      }
      else if ($result->length != 0) { // Add element to its parents.
        $created_nodes = array();
        for ($i = 0; $i < $result->length; $i++) {
          $parent_node = $result->item($i);
          $created_nodes[] = $this->createNode($xml, $value, $node);
        }
        return $created_nodes;
      }
      else if ($force_create) {
        return array($this->createNode($xml, $values, $this)); // Create root...
      }
    }
  }

  /**
   * Modifies and existing node.
   *
   * @param DOMNode $node
   *   The DOMNode to modify.
   * @param string $value
   *   The new value the node will have.
   *
   * @return DOMNode
   *   The modified DOMNode.
   */
  private function modifyNode($node, $value) {
    switch (get_class($node)) {
      case 'DOMElement':
        $node->nodeValue = $value;
        break;
      case 'DOMAttr':
        $node->value = $value;
        break;
    }
    return $node;
  }

  /**
   * Creates a DOMNode, and appends it to the specified parent.
   *
   * @param string $xml
   *   
   * @param DOMNode $parent
   * @param string $value
   *
   * @return DOMNode
   *   The newly created DOMNode.
   */
  private function createNode($xml, $parent, $value) {
    $xml = sttr($xml, '%value%', $value);
    $node = $this->createDocumentFragment();
    $node->appendXML($xml);
    return $parent->appendChild($node);
  }

}
<?php

// $Id$

/**
 * @file
 * 
 * Repersents a Fedora based XML datastream, extends DOMDocument.
 *
 * This class should never be instantiated. It is meant to provide most of the
 * common functionality that all XML based datastreams share. This class and 
 * its decendants are used to generate XML documents from submitted Drupal forms.
 * Specifically the forms that this class processes must be created by the
 * IngestFormBuilder/EditFormBuilders classes or one of thier decendants.
 */
module_load_include('inc', 'islandora_form_builder', 'FormPreprocessor');

/**
 *
 */
class XmlDatastreamFormDocument extends DOMDocument {

  /**
   * The drupal form the user has submitted.
   * @var array
   */
  protected $form;
  /**
   * The simplified preprocessed form.
   * @var array
   */
  protected $preprocessedForm;
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
    $this->formState = &$form_state;
    $this->storage = &$form_state['storage']['form_builder']; // Storage specific to form builder.
    $this->formValues = &$form_state['values']['form_builder'];
    $this->form = &$form[FORM_ROOT];
    $form_preprocessor = new FormPreprocessor($this->form, $this->formValues);
    $this->preprocessedForm = $form_preprocessor->process();
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
      $this->createDocumentFromForm($this->preprocessedForm, $this);
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
   */
  private function createDocumentFromForm(&$form, &$parent_node) {
    foreach ($form as $index => $form_element) { // This loop may change based on values ...
      $node = $this->createNodeFromForm($form_element, $parent_node);
      foreach ($form_element['#children'] as $child) {
        $this->createDocumentFromForm($child, $node);
      }
    }
  }

  /**
   * Creates a node.
   *
   * @param array $form
   * @param DOMNode $parent_node
   * @return DOMNode
   */
  private function createNodeFromForm($form, $parent_node) {
    $properties = $form['#form_builder'];
    $value = $form['#value'];
    list($path, $is_full_path) = $properties['path'];
    if ($is_full_path) {
      list($parent_path, $force_create) = $properties['parent_path'];
    }
    if ($is_full_path) {
      $result = $this->xpath->query($path);
      if ($result->length == 0) { // Create.
        //parent
        $result = $this->xpath->query($parent_path);
        if ($result->length == 0 && $force_create) {
          $node = $this->createNode($properties, $value);
          return $parent_node->appendChild($node);
        }
        else if ($result->length == 1) {
          $node = $this->createNode($properties, $value);
          return $result->item(0)->appendChild($node);
        }
      }
      else if ($result->length == 1) { // Found it.
        return $this->modifyNode($result->item(0), $value);
      }
    }
    else {
      $result = $this->xpath->query($path, $parent_node);
      if ($result->length == 0) {
        $node = $this->createNode($properties, $value);
        return $parent_node->appendChild($node);
      }
      else if ($result->length == 1) {
        return $this->modifyNode($result->item(0), $value);
      }
    }
  }

  /**
   * Creates a node.
   *
   * @param array $properties
   * @param string $value
   * @return DOMNode
   */
  private function createNode($properties, $value) {
    $xml = isset($properties['xml']) ? $properties['xml'] : NULL;
    $element = isset($properties['element']) ? $properties['element'] : NULL;
    $attribute = isset($properties['attribute']) ? $properties['attribute'] : NULL;
    if ($xml) {
      $xml = sttr($xml, '%value%', $value);
      $node = $this->createDocumentFragment();
      $node->appendXML($xml);
      return $node;
    }
    else if ($element) {
      list($prefix, $name) = explode(':', $element);
      if (isset($prefix) && isset($name)) {
        return $this->createElementNS($this->nameSpaces[$prefix], $name, $value);
      }
      else {
        return $this->createElement($element, $value);
      }
    }
    else { // Attribute
      list($prefix, $name) = explode(':', $attribute);
      if (isset($prefix) && isset($name)) {
        $attribute = $this->createAttributeNS($this->nameSpaces[$prefix], $name);
        $attribute->value = $value;
      }
      else {
        $attribute = $this->createAttribute($attribute, $value);
        $attribute->value = $value;
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

}
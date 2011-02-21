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
module_load_include('inc', 'islandora_form_builder', 'FormBuilderInterface');

/**
 *
 */
abstract class XmlDatastreamFormDocument extends DOMDocument {

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
   * A string that repersents the supported namespaces as XML attributes, used when creating document fragments.
   * @var string
   */
  protected $nameSpaceAttributes;
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
    $form_preprocessor = new FormPreprocessor($this->form);
    $this->preprocessedForm = $form_preprocessor->process();
    $this->formatOutput = TRUE;
    $this->xpath = new DOMXPath($this);
    $this->nameSpaces = $namespaces_to_register;
    foreach ($this->nameSpaces as $prefix => $uri) { // Default namespaces not supported yet...
      $this->xpath->registerNamespace($prefix, $uri);
      $this->nameSpaceAttributes .= "xmlns:$prefix = \"$uri\" ";
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
      $this->modifyExistingDocument($document, $this->preprocessedForm);
      $this->loadXML($document->saveXML()); // Is importing quicker?
    }
    else {
      $root = $this->createRootElement();
      $this->appendChild($root);
      $this->createDocumentFromForm($this->preprocessedForm, $root);
    }
  }

  /**
   * Implemented in the child class this function creates the root element.
   *
   * @return DOMElement
   *  The root element.
   */
  abstract protected function createRootElement();

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
   * @param array $form
   *   Submitted form.
   */
  private function modifyExistingDocument(&$document, &$form) {
    // Build a list of the remaining 
    // Remove element by unused hashes.
  }

  /**
   * Creates this document from submitted form values and any relevant stored data.
   */
  private function createDocumentFromForm(&$form, &$parent_node) {
    foreach ($form as $index => $form_element) { // This loop may change based on values ...
      $node = $this->createOrModifyNode($form_element, $parent_node);
      if (isset($form_element['#children'])) {
        $this->createDocumentFromForm($form_element['#children'], $node);
      }
    }
  }

  /**
   * Looks for a node described in #form_builder, if found it is modified,
   * otherwise a new node will be created from the description in #form_builder.
   *
   * @param array $form
   * @param DOMNode $parent_node
   * 
   * @return DOMNode
   */
  private function createOrModifyNode($form, $parent_node) {
    $properties = $form['#form_builder'];
    $value = isset($form['#value']) ? $form['#value'] : '';
    $node = $this->findNode($properties, $parent_node);
    if ($node) { // Found
      return $this->modifyNode($node, $value);
    }
    else { // Create
      $parent = $this->findParentNode($properties, $parent_node);
      return $this->createNode($properties, $value, $parent);
    }
  }

  /**
   * Finds a node.
   *
   * @param array $properties
   * @param DOMNode $parent_node
   *
   * @return DOMNode
   *   DOMNode if found, FALSE otherwise.
   */
  private function findNode($properties, $parent_node) {
    list($path, $full) = $properties['path'];
    if ($full) {
      $result = $this->xpath->query($path);
      if($result == FALSE) {
        $break_here;
      }
      return $result->length == 1 ? $result->item(0) : FALSE;
    }
    $result = $this->xpath->query($path, $parent_node);
    return $result->length == 1 ? $result->item(0) : FALSE;
  }

  /**
   * Find this elements parent node.
   *
   * @param array $properties
   * @param DOMNode $parent_node
   *
   * @return DOMNode
   *   The parent node.
   */
  private function findParentNode($properties, $parent_node) {
    list($path, $full) = $properties['path'];
    return $full ? $this->xpath->query($properties['parent_path'])->item(0) : $parent_node;
  }

  /**
   * Creates a node.
   *
   * @param array $properties
   * @param string $value
   * @param DOMNode $parent
   * 
   * @return DOMNode
   */
  private function createNode($properties, $value, $parent) {
    if (isset($properties['xml'])) {
      return $this->createXmlNode($properties['xml'], $value, $parent);
    }
    else if (isset($properties['element'])) {
      return $this->createElementNode($properties['element'], $value, $parent);
    }
    else if (isset($properties['attribute'])) { // Attribute
      return $this->createAttributeNode($properties['attribute'], $value, $parent);
    }
  }

  /**
   * Creates a DOMNode from an xml string, and a form value.
   *
   * @param string $xml
   * @param string $value
   * @param DOMNode $parent
   * 
   * @return DOMNode
   */
  private function createXmlNode($xml, $value, $parent) {
    // Sigh... Oh DOM-API, your so backwards and useless.
    // We have to do this for namespaces to work correctly, there is no clean work around for this.
    $xml = strtr($xml, array('%value%' => $value));
    $xml = "<root {$this->nameSpaceAttributes}>$xml</root>";
    $node = $this->createDocumentFragment();
    $node->appendXML($xml);
    return $parent->appendChild($node->childNodes->item(0)->childNodes->item(0));
  }

  /**
   * Creates an DOMElement $element, with a textContent of $value.
   *
   * @param string $element
   * @param string $value
   * @param DOMNode $parent
   *
   * @return DOMElement
   */
  private function createElementNode($element, $value, $parent) {
    if (strpos($element, ':') !== FALSE) {
      list($prefix, $name) = explode(':', $element);
      $node = $this->createElementNS($this->nameSpaces[$prefix], $element, $value);
      return $parent->appendChild($node);
    }
    else {
      $node = $this->createElement($element, $value);
      return $parent->appendChild($node);
    }
  }

  /**
   * Creates an DOMAttr $attribute, with a value of $value.
   *
   * @param string $attribute
   * @param string $value
   * @param DOMNode $parent
   *
   * @return DOMAttr
   */
  private function createAttributeNode($attribute, $value, $parent) {
    if (strpos($attribute, ':') !== FALSE) {
      list($prefix, $name) = explode(':', $attribute);
      return $parent->setAttributeNS($this->nameSpaces[$prefix], $attribute, $value);
    }
    else {
      return $parent->setAttribute($attribute, $value);
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
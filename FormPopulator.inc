<?php

// $Id$

/**
 * @file
 *
 * Contains a class that populates form values from the specified fedora object.
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');
module_load_include('inc', 'islandora_form_builder_elements', 'Utilities');

/**
 * Populates the values of a drupal form using #form_builder attributes, and the fedora object's
 * Content Model.
 */
class FormPopulator {

  /**
   * The fedora object to edit, datastreams from this item are used to populate the form.
   * 
   * @var Fedora_Item
   */
  protected $item;
  /**
   * The datastream we are editing. This is used to populate existing form elements.
   * 
   * @var DOMDocument
   */
  protected $xml;
  /**
   * The state of the drupal form.
   *
   * @var array
   */
  protected $formState;
  /**
   * Persistant data used by the form builder module.
   * 
   * @var array
   */
  protected $storage;
  /**
   * A list of the allowed drupal form elements.
   *
   * @var array
   */
  protected $allowedFormElements;
  /**
   * The default prefix for the default namespaces of the document.
   * @var string
   */
  protected $defaultPrefix;

  /**
   * Generates a hash and xpath that can be used to identify this node.
   * 
   * @param DOMNode $node
   *   Node to generate the hash/path for.
   * @param string $default_prefix
   *   Default prefix to use if the node does not have one.
   *
   * @return array
   *   An two index array where the first is the hash of the node and the second is an xpath to the node.
   */
  public static function generateHashPath(&$node, $default_prefix) {
    $path = self::getNodeXPath($node, $default_prefix);
    $index = self::findXPathIndexInParentElement($node);
    $path = get_class($node) == 'DOMAttr' ? "@$path" : "{$path}[{$index}]";
    $parent = $node;
    while (($parent = $parent->parentNode) && (get_class($parent) == 'DOMElement')) {
      $parent_path = self::getNodeXPath($parent, $default_prefix);
      $index = self::findXPathIndexInParentElement($parent);
      $path = "{$parent_path}[$index]/$path";
    }
    $path = "/$path";
    return array(md5($path), $path);
  }

  /**
   * Find's the index of the child within it's parent element.
   * Base 1 index, since it will be used by xpath.
   *
   * @param DOMNode $node
   * 
   * @return int
   */
  private static function findXPathIndexInParentElement(&$node) {
    $count = 1;
    $name = $node->nodeName;
    $next = $node;
    while ($next = $next->previousSibling) {
      if (get_class($next) == 'DOMElement' && $next->nodeName == $name) {
        $count++;
      }
    }
    return $count;
  }

  private static function getNodeXPath(&$node, &$default_prefix) {
    $node->nodeName;
    if($node->prefix == '' && get_class($node) != 'DOMAttr') {
      return "{$default_prefix}:{$node->nodeName}";
    }
    else {
      return $node->nodeName;
    }
  }

  /**
   * Creates a FormPopulator.
   *
   * @param array $form_state
   *   The state of the drupal form.
   * @param string $pid
   *   The pid of the fedora object to be edited/used for populating form fields.
   * @param string $dsid
   *   The datastream ID of the datastream to be edited.
   */
  function __construct(&$form_state, $pid, $dsid, $form_name) {
    $this->initializeStorage($form_state, $pid, $dsid);
    $this->xml = new DOMDocument();
    $this->xml->loadXML($this->storage['xml']);
    $this->xpath = new DOMXPath($this->xml);
    $this->registerNamespaces($form_name);
    $this->allowedFormElements = islandora_form_builder_elements_get_allowed_form_elements();
  }

  /**
   *
   * @param array $form_state
   * @param string $pidUt
   * @param string $dsid
   */
  private function initializeStorage(&$form_state, $pid, $dsid) {
    $this->formState = &$form_state;
    $this->storage = &$this->formState['storage'][STORAGE_KEY];
    if (!isset($this->storage['xml'])) {
      $this->item = new fedora_item($pid);
      $this->storage['xml'] = $this->item->get_datastream_dissemination($dsid);
    }
  }

  /**
   *
   * @param string $form_name
   */
  private function registerNamespaces(&$form_name) {
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    $content_model = ContentModel::loadFromObject($this->item->pid);
    $form = $content_model->getForm($form_name);
    $module = $form->getAttribute('document_module');
    $file = $form->getAttribute('document_file');
    $class = $form->getAttribute('document_class');
    $path = drupal_get_path('document_module', $module);
    require_once "$path/$file";
    list($default_prefix) = call_user_func(array($class, "getDefaultNamespace"));
    $this->defaultPrefix = $default_prefix;
    $namespaces = call_user_func(array($class, "getRequiredNamespaces"));
    foreach ($namespaces as $prefix => $uri) { // Default namespaces not supported yet...
      $this->xpath->registerNamespace($prefix, $uri);
    }
  }

  /**
   * Uses the existing datastream to populate for elements.
   * 
   * @param array $form
   */
  public function populate(&$form) {
    $parent_node = $this->xml->documentElement;
    $this->populateFormElements($form, $parent_node);
  }

  /**
   * 
   * @param array $form
   */
  private function populateFormElements(&$form, &$parent_node) {
    foreach ($form as &$form_element) {
      if ($this->isFormElement($form_element)) {
        $this->populateFormElement($form_element, $parent_node);
        if ($this->hasChildFormElements($form_element)) {
          $this->populateChildFormElements($form_element, $parent_node);
        }
      }
    }
  }

  /**
   * Checks to see if a form element can be processed.
   *
   * @param array $form
   *   Drupal form.
   *
   * @return boolean
   *   TRUE if this $form can be processed false otherwhise.
   */
  private function isFormElement(&$form) {
    $has_type = isset($form['#type']);
    return $has_type ? array_search($form['#type'], $this->allowedFormElements) !== FALSE : FALSE;
  }

  /**
   * Checks if the form element $form, has child elements.
   *
   * @param array $form
   * 
   * @return boolean
   *   TRUE if the form element has children FALSE otherwise.
   */
  private function hasChildFormElements(&$form) {
    return isset($form['content']) ? TRUE : FALSE;
  }

  /**
   * Recursively populates all the child elements.
   *
   * @param array $form
   * @param DOMNode $parent_node
   */
  private function populateChildFormElements(&$form, &$parent_node) {
    $content = &$form['content'];
    $properties = isset($form['#form_builder']) ? $form['#form_builder'] : NULL;
    if ($properties) {
      $node_list = $this->queryPath($properties, $parent_node);
      for ($i = 0; $i < $node_list->length; $i++) {
        $node = $node_list->item($i);
        $this->populateFormElements($content[$i], $node);
      }
    }
    else { // Not all elements have to have form_builder properties
      foreach ($content as &$child) {
        $this->populateFormElements($child, $parent_node);
      }
    }
  }

  /**
   *
   * @param array $form 
   */
  private function populateFormElement(&$form, &$parent_node) {
    $type = $form['#type'];
    $properties = isset($form['#form_builder']) ? $form['#form_builder'] : NULL;
    if ($properties) {
      $node_name = $parent_node->nodeName;
      $node_list = $this->queryPath($properties, $parent_node);
      $registered_nodes = $this->registerNodes($node_list);
      if (count($registered_nodes) > 0) {
        $this->populateType($type, $form, $registered_nodes);
      }
    }
  }

  /**
   *
   * @param array $properties
   * @param DOMNode $parent_node
   * @return DOMNodeList
   */
  private function queryPath(&$properties, &$parent_node) {
    list($path, $full) = $properties['path'];
    return $full ? $this->xpath->query($path) : $this->xpath->query($path, $parent_node);
  }

  /**
   * Register all the nodes that are used to populate elements.
   *
   * @param DOMNodeList $node_list
   *
   * @return array
   *   An map of the generated hashes and their respective DOMNodes.
   */
  private function registerNodes(&$node_list) {
    $registered_nodes = array();
    for ($i = 0; $i < $node_list->length; $i++) {
      $node = $node_list->item($i);
      list($hash, $path) = self::generateHashPath($node, $this->defaultPrefix);
      $this->registerHashPath($hash, $path);
      $registered_nodes[] = array($hash, $node);
    }
    return $registered_nodes;
  }

  /**
   *
   * @param DOMNode $node
   * @param hash $hash
   */
  private function registerHashPath($hash, $path) {
    $this->storage['hashes'][$hash] = $path;
  }

  /**
   *
   * @param <type> $type
   * @param <type> $registered_nodes
   */
  private function populateType($type, &$form, &$registered_nodes) {
    list($class, $filename) = islandora_form_builder_elements_get_class_name_and_file_name_of_form_element_type($type);
    if ($filename != '') {
      require_once $filename;
      if (is_callable("$class::populate")) {
        call_user_func_array(array($class, "populate"), array(&$form, &$registered_nodes));
        return;
      }
    }
    $this->defaultPopulateType($form, $registered_nodes);
  }

  /**
   *
   * @param <type> $form
   * @param <type> $registered_nodes
   */
  private function defaultPopulateType(&$form, &$registered_nodes) {
    list($hash, $node) = $registered_nodes[0];
    $form['#default_value'] = trim($node->textContent);
    $form['#form_builder']['hash'] = $hash;
  }

}
<?php

// $Id$

/**
 * @file
 *
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');
module_load_include('inc', 'islandora_form_builder_elements', 'Utilities');
module_load_include('inc', 'fedora_repository', 'ContentModel');

/**
 *
 */
class FormPopulator {

  protected $item;
  protected $xml;
  protected $formState;
  protected $storage;
  protected $allowedFormElements;

  /**
   * Generates a hash that can be used to identify this node.
   * 
   * @param DOMNode $node
   * 
   * @return hash
   */
  public static function generateHash(&$node) {
    return 0;
  }

  /**
   *
   * @param array $form_state
   * @param string $pid
   * @param string $dsid
   */
  function __construct(&$form_state, $pid, $dsid) {
    $this->initializeStorage($form_state, $pid, $dsid);
    $this->xml = $this->storage['xml'];
    $this->xpath = new DOMXPath($this->xml);
    module_load_include('inc', 'islandora_form_builder', 'ModsDatastreamForm');
    $namespaces = ModsDocument::getRequiredNamespaces();
    foreach ($namespaces as $prefix => $uri) {
      $this->xpath->registerNamespace($prefix, $uri);
    }
    $this->allowedFormElements = islandora_form_builder_elements_get_allowed_form_elements();
  }

  /**
   *
   * @param array $form_state
   * @param string $pidUt
   * @param string $dsid
   */
  private function initializeStorage(&$form_state, $pid, $dsid) {
    $this->formState = $form_state;
    $this->storage = $this->formState['storage'][STORAGE_KEY];
    if (!isset($this->storage['xml'])) {
      $this->item = new fedora_item($pid);
      $xml_string = $this->item->get_datastream_dissemination($dsid);
      $this->storage['xml'] = new DOMDocument();
      $this->storage['xml']->loadXML($xml_string);
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
    return $has_type ? array_search($form['#type'], $this->allowedFormElements) : FALSE;
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
      $node_list = $this->queryPath($properties, $parent_node);
      $this->registerNodes($node_list);
      $this->populateType($type, $form, $node_list);
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
   *
   * @param DOMNodeList $node_list
   */
  private function registerNodes(&$node_list) {
    for ($i = 0; $i < $node_list->length; $i++) {
      $node = $node_list->item($i);
      $hash = self::generateHash($node);
      $this->registerNodeWithHash($node, $hash);
    }
  }

  /**
   *
   * @param DOMNode $node
   * @param hash $hash
   */
  private function registerNodeWithHash(&$node, $hash) {
    $this->storage['hashes'][$hash] = $node;
  }

  /**
   *
   * @param <type> $type
   * @param <type> $node_list
   */
  private function populateType($type, &$form, &$node_list) {
    list($class, $filename) = islandora_form_builder_elements_get_class_name_and_file_name_of_form_element_type($type);
    require_once $filename;
    if (method_exists($class, "populate")) {
      call_user_func_array(array($class, "populate"), array(&$form, &$node_list));
    }
  }

}
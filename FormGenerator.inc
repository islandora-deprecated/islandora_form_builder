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
 * Creates a Drupal form from a XML form definition stored in a Content Model.
 */
class FormGenerator {

  protected $formDefinition;
  protected $xpath;
  protected $formElementTypes;

  function __construct($content_model_pid, $content_model_dsid) {
    $this->contentModel = ContentModel::loadFromModel($content_model_pid, $content_model_dsid);
    $xml = $this->contentModel->dumpXml();
    $this->formDefinition = new DOMDocument();
    //$this->formDefinition->loadXML($xml);
    $filename = drupal_get_path('module', 'islandora_form_builder') . '/test.xml';
    $this->formDefinition->load($filename);
    // End Convience.
    $this->xpath = new DOMXPath($this->formDefinition);
    $this->xpath->registerNamespace('cm', "http://www.islandora.ca");
    $this->formElementTypes = islandora_form_builder_elements_get_allowed_form_elements();
  }

  public function generateForm($form_name) {
    $form_element = $this->findFormElement($form_name);
    if ($form_element) {
      $form = array();
      $result = $this->xpath->query('child::*', $form_element);
      $form[FORM_ROOT]['#tree'] = TRUE;
      for ($i = 0; $i < $result->length; $i++) { // Top level form elements.
        $child = $result->item($i);
        $form[FORM_ROOT][] = $this->createDrupalFormElement($child);
      }
      return $form;
    }
    return FALSE;
  }

  private function findFormElement($form_name) {
    $result = $this->xpath->query("//cm:form[@name='$form_name']");
    if ($result->length > 1) {
      drupal_set_message(t("Form names should be unique. Check the Content Modeler to ensure that form: $form_name is unique."), 'error');
      return FALSE;
    }
    else if ($result->length == 0) {
      drupal_set_message(t("Could not find form: $form_name. Please Check the Content Modeler to ensure that form: $form_name exists."), 'error');
      return FALSE;
    }
    else {
      return $result->item(0);
    }
  }

  private function createDrupalFormElement(&$form_element) {
    $type = $form_element->tagName;
    $form = array('#type' => $type);
    $result = $this->xpath->query('child::*', $form_element);
    for ($i = 0; $i < $result->length; $i++) {
      $child = $result->item($i);
      list($name, $value) = $this->createDrupalFormControl($child);
      $form[$name] = $value;
    }
    $method_name = 'alter' . str_replace('_', '', $type) . 'FormElement';
    if (method_exists($this, $method_name)) {
      call_user_func_array(array(&$this, $method_name), array(&$form));
    }
    return $form;
  }

  private function createDrupalFormControl(&$form_element) {
    $name = $form_element->tagName;
    $method_name = 'create' . str_replace('_', '', $name) . 'FormControl';
    if (method_exists($this, $method_name)) {
      return call_user_func_array(array(&$this, $method_name), array(&$form_element));
    }
    else { // Default case the text in the element is its value.
      return array("#$name", $form_element->textContent);
    }
  }

  private function createContentFormControl(&$form_element) {
    $content = array();
    $result = $this->xpath->query('child::*', $form_element);
    for ($i = 0; $i < $result->length; $i++) {
      $child = $result->item($i);
      $content[0][] = $this->createDrupalFormElement($child);
    }
    return array('content', $content);
  }

  private function createOptionsFormControl(&$form_element) {
    // Get all options
    $options = array();
    $result = $this->xpath->query('child::*', $form_element);
    for ($i = 0; $i < $result->length; $i++) {
      $child = $result->item($i);
      $options[] = $child->textContent;
    }
    // Should we use the first option as a prompt?
    $ignore_first_option = $form_element->getAttribute('ignore_first_element') == 'true';
    if ($ignore_first_option) {
      $select_message = array_shift($options);
      $options = array_merge(array(NULL => $select_message), $options);
    }
    return array('#options', $options);
  }

  private function createFormBuilderFormControl(&$form_element) {
    $form_builder = array();
    $path = $this->xpath->query('descendant::cm:path', $form_element)->item(0);
    $full = $path->getAttribute('full') == 'true';
    $form_builder['path'] = array($path->textContent, $full);
    if ($full) {
      $parent_path = $this->xpath->query('descendant::cm:parent_path', $form_element)->item(0);
      $form_builder['parent_path'] = array($parent_path->textContent, $parent_path->getAttribute('force_create_if_not_found') == 'true');
    }
    $xml = $this->xpath->query('descendant::cm:xml', $form_element);
    if ($xml->length == 1) {
      $form_builder['xml'] = $xml->item(0)->textContent;
    }
    $element = $this->xpath->query('descendant::cm:element', $form_element);
    if ($element->length == 1) {
      $form_builder['element'] = $element->item(0)->textContent;
    }
    $attribute = $this->xpath->query('descendant::cm:attribute', $form_element);
    if ($attribute->length == 1) {
      $form_builder['attribute'] = $attribute->item(0)->textContent;
    }
    $require_value = $this->xpath->query('descendant::cm:require_value', $form_element)->item(0);
    $form_builder['require_value'] = $require_value->textContent;
    return array('#form_builder', $form_builder);
  }

  private function alterTabPanelFormElement(&$form) {
    $form['#form_builder'];
    $form['content'][0]['#form_builder'] = $form['#form_builder'];
    $form['content'][0]['#form_builder']['path'][0] .= '[1]';
  }
}

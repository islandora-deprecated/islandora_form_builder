<?php

// $Id$

/**
 * @file
 *
 * Contains a class that generates a drupal from from a form definition stored in a content model.
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');
module_load_include('inc', 'islandora_form_builder_elements', 'Utilities');
module_load_include('inc', 'fedora_repository', 'ContentModel');

/**
 * Creates a Drupal form from a XML form definition stored in a Content Model.
 */
class FormGenerator {

  /**
   * Content Model's XML that contains the form's definition.
   *
   * @var string
   */
  protected $formDefinition;
  /**
   * DOMXPath for the $formDefinition.
   *
   * @var DOMXPath
   */
  protected $xpath;
  /**
   * An array of allowed drupal form element types.
   *
   * @var array
   */
  protected $formElementTypes;

  /**
   * Creates a FormGenerator from an object.
   * 
   * @param string $pid
   * 
   * @return FormGenerator 
   */
  public static function CreateFromObject($pid) {
    $content_model = ContentModel::loadFromObject($pid);
    return new FormGenerator($content_model);
  }

  /**
   * Creates a FormGenerator from a model.
   *
   * @param string $content_model_pid
   * @param string $content_model_dsid
   *
   * @return FormGenerator 
   */
  public static function CreateFromModel($content_model_pid, $content_model_dsid) {
    $content_model = ContentModel::loadFromModel($content_model_pid, $content_model_dsid);
    return new FormGenerator($content_model);
  }

  /**
   * Create the FormGenerator.
   * 
   * @param ContentModel $content_model
   */
  private function __construct($content_model) {
    $this->contentModel = $content_model;
    $xml = $this->contentModel->dumpXml();
    $this->formDefinition = new DOMDocument();
    $this->formDefinition->loadXML($xml);
    $this->xpath = new DOMXPath($this->formDefinition);
    $this->xpath->registerNamespace('cm', "http://www.islandora.ca");
    $this->formElementTypes = islandora_form_builder_elements_get_allowed_form_elements();
  }

  /**
   * Generates a drupal form based on a XML definition in the content model.
   *
   * @param string $form_name
   *
   * @return array
   *   Drupal Form.
   */
  public function generate($form_name) {
    $form_element = $this->findFormElement($form_name);
    if ($form_element) {
      $form = array();
      $result = $this->xpath->query('child::*', $form_element);
      $form['#tree'] = TRUE;
      for ($i = 0; $i < $result->length; $i++) { // Top level form elements.
        $child = $result->item($i);
        $form[] = $this->createDrupalFormElement($child);
      }
      return $form;
    }
    return FALSE;
  }

  /**
   * Finds the named form in the content models definition.
   *
   * @param string $form_name
   *   Name of the form element to retrieve.
   * 
   * @return DOMElement
   *   The form element if found, FALSE otherwise.
   */
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

  /**
   * Creates a drupal form element from a xml definition.
   *
   * @param DOMElement $form_element
   *   XML definition of a drupal form element.
   * 
   * @return array
   *   Drupal form element.
   */
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

  /**
   * Creates a drupal form control.
   *
   * @param DOMElement $form_element
   *   A XML definition for the form control.
   * 
   * @return array
   *   A drupal from control such as #title or #form_builder
   */
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

  /**
   * Creates a Content Form Control.
   *
   * @param DOMElement $form_element
   *   A XML definition for the form control.
   *
   * @return array
   *   The drupal from control.
   */
  private function createContentFormControl(&$form_element) {
    $content = array();
    $result = $this->xpath->query('child::*', $form_element);
    for ($i = 0; $i < $result->length; $i++) {
      $child = $result->item($i);
      $content[0][] = $this->createDrupalFormElement($child);
    }
    return array('content', $content);
  }

  /**
   * Creates a Options Form Control.
   *
   * @param DOMElement $form_element
   *   A XML definition for the form control.
   *
   * @return array
   *   The drupal from control.
   */
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

  /**
   * Creates a Form Builder Form Control.
   *
   * @param DOMElement $form_element
   *   A XML definition for the form control.
   *
   * @return array
   *   The drupal from control.
   */
  private function createFormBuilderFormControl(&$form_element) {
    $form_builder = array();
    $path = $this->xpath->query('descendant::cm:path', $form_element)->item(0);
    $full = $path->getAttribute('full') == 'true';
    $form_builder['path'] = array($path->textContent, $full);
    if ($full) {
      $parent_path = $this->xpath->query('descendant::cm:parent_path', $form_element)->item(0);
      $form_builder['parent_path'] = $parent_path->textContent;
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

  /**
   * Alter the generated TabPanel Form Element.
   *
   * @param DOMElement $form
   *   The Drupal Form element to be altered.
   */
  private function alterTabPanelFormElement(&$form) {
    if (isset($form['#form_builder'])) {
      $form['content'][0]['#form_builder'] = $form['#form_builder'];
      $form['content'][0]['#form_builder']['path'][0] .= '[1]';
    }
  }

  /**
   * Alter the generated FieldSet Form Element.
   *
   * @param DOMElement $form
   *   The Drupal Form element to be altered.
   */
  private function alterFieldSetFormElement(&$form) {
    if (isset($form['#form_builder'])) {
      $form['content'][0]['#form_builder'] = $form['#form_builder'];
      $form['content'][0]['#form_builder']['path'][0] .= '[1]';
    }
  }

}

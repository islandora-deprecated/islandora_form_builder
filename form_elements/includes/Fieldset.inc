<?php

// $Id$

/**
 * @file
 *
 */

/**
 * 
 */
class Fieldset {

  /**
   * A list of the allowed drupal form elements.
   *
   * @var array
   */
  protected $allowedFormElements;

  /**
   * Create a FieldSet.
   * 
   */
  public function __constructor() {
    $this->allowedFormElements = islandora_form_builder_elements_get_allowed_form_elements();
  }

  /**
   * Determines if the argument is a drupal form element.
   * 
   * @param array $form
   *   Drupal form element.
   *
   * @return boolean
   *   TRUE if $form is a form element, FALSE otherwise.
   */
  private function isFormElement(&$form) {
    $has_type = isset($form['#type']);
    return $has_type ? array_search($form['#type'], $this->allowedFormElements) !== FALSE : FALSE;
  }

  /**
   * Populates the fieldset, from an existing datastream.
   *
   * @param array $form
   *   Drupal fieldset form element.
   * @param array $registered_nodes
   *   Nodes from an existing datastream that this fieldset will repersent.
   */
  public static function populate(&$form, &$registered_nodes) {
    list($hash, $node) = $registered_nodes[0];
    $form['content'][0]['#form_builder'] = $form['#form_builder'];
    $form['content'][0]['#form_builder']['path'][0] .= "[1]"; // XPath indices start at 1
    $form['content'][0]['#form_builder']['hash'] = $hash;
  }

  /**
   * Checks if any element in $form has a value.
   * 
   * @param array $form
   *   A list of form elements.
   * 
   * @return boolean
   *   TRUE if a element has a value FALSE otherwise.
   */
  private function anyElementHasValue(&$form) {
    foreach ($form as &$form_element) {
      if ($this->isFormElement($form_element)) {
        $has_value = isset($element['#default_value']) || isset($element['#value']);
        if ($has_value) {
          return TRUE;
        }
        else if ($form_element['content']) {
          if ($this->anyElementHasValue($form_element['content'])) {
            return TRUE;
          }
        }
      }
    }
    return FALSE;
  }

  /**
   * Process the form set.
   *
   * @param array $element
   *   Drupal fieldset form Element.
   * @param array $form_state
   *   The state of the drupal form.
   *
   * @return array
   *   The processed fieldset.
   */
  public function process(&$element, &$form_state) {
    if (isset($element['#collapsed']) && isset($element['content']) && $this->anyElementHasValue($element['content'])) {
      unset($element['#collapsed']);
    }
    return $element;
  }

}

/**
 * Processes a fieldset element.
 *
 * @param array $element
 *   Drupal fieldset form element.
 * @param string $edit
 *   Not used...
 * @param array $form_state
 *   The state of the drupal form.
 *
 * @return array
 *   Modified fieldset form element.
 */
function islandora_form_builder_elements_process_fieldset(&$element, $edit, &$form_state) {
  $fieldset = new Fieldset();
  return $fieldset->process($element, $form_state);
}
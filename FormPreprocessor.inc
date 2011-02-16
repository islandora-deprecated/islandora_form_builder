<?php

// $Id$

/**
 * @file
 *
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');
module_load_include('inc', 'islandora_form_builder_elements', 'Utilities');

/**
 * 
 */
class FormPreprocessor {

  /**
   * The form to process. This is rooted at FORM_ROOT.
   *
   * @var array
   */
  protected $form;

  /**
   * Create a FormPreprocessor.
   *
   * @param array $form
   * @param array $form_state
   */
  public function __construct(&$form) {
    $this->form = &$form;
    $this->allowedFormElements = islandora_form_builder_elements_get_allowed_form_elements();
  }

  /**
   * Process all child elements.
   *
   * @param array $form
   * @param array $form_values
   * 
   * @return array
   */
  private function processChildren(&$form, &$parent) {
    foreach ($form as $form_element) {
      if ($this->canBeProcessed($form_element)) {
        $this->processChildFormElement($form_element, $parent);
      }
    }
  }

  /**
   * Process a child form element.
   *
   * @param array $form
   *
   * @param array $parent
   */
  private function processChildFormElement(&$form, &$parent) {
    if (isset($form['content'])) {
      $this->processChildContent($form['content'], $parent);
    }
    else {
      $properties = $this->getFormBuilderProperties($form);
      if ($properties) {
        $parent['#children'][] = array('#form_builder' => $properties, '#value' => $form['#value']);
      }
    }
  }

  /**
   * Process a form element that has no parents.
   *
   * @param array $form
   *
   * @return array
   */
  private function processFormElement(&$form) {
    if (isset($form['content'])) {
      return $this->processContent($form['content']);
    }
    else {
      $properties = $this->getFormBuilderProperties($form);
      if ($properties) {
        $form_element = array('#form_builder' => $properties, '#value' => $form['#value']);
        return array($form_element);
      }
    }
  }

  /**
   * Process a the content of a form element.
   *
   * @param array $content
   *
   * @param array $parent
   */
  private function processChildContent(&$content, &$parent) {
    $i = 0;
    while (isset($content[$i])) {
      $preprocessed_form['#form_builder'] = $this->getFormBuilderProperties($content[$i]);
      $this->preprocess($content[$i], $preprocessed_form);
      $parent['#children'][] = $preprocessed_form;
      $i++;
    }
  }

  /**
   * Process a the content of a parentless form element.
   * 
   * @param array $content
   *
   * @return array
   */
  private function processContent(&$content) {
    $processed_content;
    $i = 0;
    while (isset($content[$i])) {
      $properties = $this->getFormBuilderProperties($content[$i]);
      $parent['#form_builder'] = $properties;
      $this->processChildren($content[$i], $parent);
      $processed_content[] = $parent;
      $i++;
    }
    return $processed_content;
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
  private function canBeProcessed(&$form) {
    $has_type = isset($form['#type']);
    return $has_type ? array_search($form['#type'], $this->allowedFormElements) : FALSE;
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
   * Process form.
   *
   * @return array
   */
  private function processForm() {
    $processed_form = array();
    foreach ($this->form as $form) {
      if ($this->canBeProcessed($form)) {
        $processed_form = array_merge($processed_form, $this->processFormElement($form));
      }
    }
    return $processed_form;
  }

  /**
   * Process the form.
   *
   * @return array
   *
   */
  public function process() {
    $processed_form = $this->processForm();
    return $this->prune($processed_form);
  }

  /**
   * Remove branches of the processed form, if they are not required,
   * and if they don't have valid values.
   *
   * @param array $processed_form
   *   The already procesed form.
   * 
   * @return array
   *   The pruned processed form.
   */
  private function prune(&$processed_form) {
    foreach ($processed_form as $i => $element) {
      if ($this->shouldBePruned($element)) {
        unset($processed_form[$i]);
      }
    }
    return $processed_form;
  }

  /**
   * Determine if a branch should be pruned.
   *
   * Form elements that have no children are pruned if they have no value and they require a value.
   * Form elements with children are pruned if the element is not required and if all of its child
   * elements have been pruned.
   *
   * @param array $branch
   *   Branch to check.
   * @return boolean
   *   TRUE if this branch should be pruned FALSE otherwise.
   */
  private function shouldBePruned($branch) {
    $has_children = isset($branch['#children']);
    if (!$has_children) {
      $properties = $this->getFormBuilderProperties($branch);
      $value = $branch['#value'];
      $require_value = $properties['require_value'];
      $valid_value = is_string($value) && (!$require_value) ? TRUE : trim($value) !== '';
      return!$valid_value;
    }
    else {
      $this->prune($branch['#children']);
      return count($branch['#children']) == 0 && !$properties['#form_builder']['require_value'];
    }
  }

}
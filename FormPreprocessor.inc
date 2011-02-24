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
   * Process the form.
   *
   * @return array
   * 
   */
  public function &process() {
    $processed_form = &$this->processForm();
    $processed_form = $this->prune($processed_form);
    return $processed_form;
  }

  /**
   * Process form.
   *
   * @return array
   */
  private function &processForm() {
    $processed_form = array();
    $this->processFormElements($this->form, $processed_form);
    return $processed_form;
  }

  /**
   *
   * @param <type> $form_elements
   * @param <type> $processed_form
   */
  private function processFormElements(&$form_elements, &$processed_form) {
    foreach ($form_elements as &$form) {
      if ($this->canBeProcessed($form)) {
        $this->processFormElement($form, $processed_form);
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
  private function canBeProcessed(&$form) {
    $has_type = isset($form['#type']);
    return $has_type ? array_search($form['#type'], $this->allowedFormElements) !== FALSE : FALSE;
  }

  /**
   * Process a form element that has no parents.
   *
   * @param array $form
   * @param array $processed_form
   *
   * @return array
   */
  private function processFormElement(&$form, &$processed_form) {
    isset($form['content']) ?
            $this->processContent($form, $processed_form) :
            $this->processFormElementType($form, $processed_form);
  }

  /**
   * Process a the content of a parentless form element.
   *
   * @param array $form
   * @param array $processed_form
   */
  private function processContent(&$form, &$processed_form) {
    $count = 1;
    $content = $form['content'];
    foreach ($content as $key => &$form_elements) {
      if (is_numeric($key)) {
        $properties = $this->getFormBuilderProperties($form_elements);
        if ($properties) {
          $form_element['#form_builder'] = $properties;
          $form_element['#form_builder']['path'][0] = "mostunli"; // We Want the path to fail, so that a new object is created when no hash exists.
          $form_element['#children'] = array();
          $this->processFormElements($form_elements, $form_element); // Root child elements.
          $this->addToProcessedForm($form_element, $processed_form);
        }
        else {
          $this->processFormElements($form_elements, $processed_form);
        }
      }
      $count++;
    }
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
    return (isset($form['#form_builder']) && is_array($form['#form_builder'])) ? $form['#form_builder'] : FALSE;
  }

  /**
   *
   * @param array $form
   * @param array $processed_form
   */
  private function processFormElementType(&$form, &$processed_form) {
    $type = $form['#type'];
    $func = 'process' . $type . 'Element';
    if (is_callable(array($this, $func))) {
      call_user_func_array(array($this, $func), array(&$form, &$processed_form));
    }
    else {
      $this->defaultProcessElement($form, $processed_form);
    }
  }

  /**
   *
   * @param <type> $form
   * @param <type> $processed_form
   */
  private function processSetElement(&$form, &$processed_form) {
    $properties = $this->getFormBuilderProperties($form);
    if ($properties) {
      $values = explode(';', $form['#value']);
      $hashes = isset($properties['hash']) ? $properties['hash'] : array();
      $i = 1; // XPath starts at 1
      while ($value = array_shift($values)) {
        $form_builder = $properties;
        $form_builder['path'][0] .= "[$i]";
        $hash = array_shift($hashes);
        if (isset($hash)) {
          $form_builder['hash'] = $hash;
        }
        $this->addToProcessedForm(array('#form_builder' => $form_builder, '#value' => $value), $processed_form);
        $i++;
      }
    }
  }

  /**
   * 
   * @param <type> $form
   * @param <type> $processed_form
   */
  private function defaultProcessElement(&$form, &$processed_form) {
    $properties = $this->getFormBuilderProperties($form);
    if ($properties) {
      $value = isset($form['#value']) ? $form['#value'] : '';
      $this->addToProcessedForm(array('#form_builder' => $properties, '#value' => $value), $processed_form);
    }
  }

  /**
   * Adds a element to the processed form.
   * @param <type> $form_element
   * @param <type> $processed_form
   */
  private function addToProcessedForm($form_element, &$processed_form) {
    $is_parent = isset($processed_form['#children']);
    if ($is_parent) {
      $processed_form['#children'][] = $form_element;
    }
    else {
      $processed_form[] = $form_element;
    }
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
    foreach ($processed_form as $i => &$element) {
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
  private function shouldBePruned(&$branch) {
    $properties = $this->getFormBuilderProperties($branch);
    $has_children = isset($branch['#children']);
    if (!$has_children) {
      if (!isset($branch['#value'])) {
        $i = 0;
        $i++;
        $break_here;
      }
      $value = $branch['#value'];
      $value_required = $properties['require_value'] == 'true';
      $valid_value = (!$value_required) || (is_string($value) && trim($value) !== '');
      return!$valid_value;
    }
    else {
      $this->prune($branch['#children']);
      return count($branch['#children']) == 0 && $properties['require_value'];
    }
  }

}
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
      $type = $form['#type'];
      $func = 'process' . $type . 'ChildElement';
      if (is_callable(array($this, $func))) {
        call_user_func_array(array($this, $func), array(&$form, &$parent));
      }
      else {
        $this->defaultProcessChildElement($form, $parent);
      }
    }
  }

  /**
   * Process a form element that has no parents.
   *
   * @param array $form
   * @param array $proccessed_form
   *
   * @return array
   */
  private function processFormElement(&$form, &$processed_form) {
    if (isset($form['content'])) {
      $this->processContent($form['content'], $processed_form);
    }
    else {
      $type = $form['#type'];
      $func = 'process' . $type . 'Element';
      if (is_callable(array($this, $func))) {
        call_user_func_array(array($this, $func), array(&$form, &$processed_form));
      }
      else {
        $this->defaultProcessElement($form, $processed_form);
      }
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
        $processed_form[] = array('#form_builder' => $form_builder, '#value' => $value);
        $i++;
      }
    }
  }

  /**
   *
   * @param <type> $form
   * @param <type> $parent
   */
  private function processSetChildElement(&$form, &$parent) {
    $properties = $this->getFormBuilderProperties($form);
    if ($properties) {
      $values = explode(';', $form['#value']);
      $hashes = isset($properties['hash']) ? $properties['hash'] : array();
      $i = 1; // XPath starts at 1
      while ($value = array_shift($values)) {
        $form_builder = $properties;
        $form_builder['path'][0] .= "[$i]";
        $hash = array_shift($hashes);
        if ($hash) {
          $form_builder['hash'] = $hash;
        }
        $parent['#children'][] = array('#form_builder' => $form_builder, '#value' => $value);
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
      $processed_form[] = array('#form_builder' => $properties, '#value' => $value);
    }
  }

  /**
   *
   * @param <type> $form
   * @param <type> $processed_form
   */
  private function defaultProcessChildElement(&$form, &$parent) {
    $properties = $this->getFormBuilderProperties($form);
    if ($properties) {
      $value = isset($form['#value']) ? $form['#value'] : '';
      $parent['#children'][] = array('#form_builder' => $properties, '#value' => $value);
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
      $properties = $this->getFormBuilderProperties($content[$i]);
      if ($properties) {
        $preprocessed_form['#form_builder'] = $properties;
        $this->processChildren($content[$i], $preprocessed_form);
        $parent['#children'][] = $preprocessed_form;
      }
      else {
        foreach ($content[$i] as $form) {
          if ($this->canBeProcessed($form)) {
            $this->processChildFormElement($form, $parent);
          }
        }
      }
      $i++;
    }
  }

  /**
   * Process a the content of a parentless form element.
   * 
   * @param array $content
   * @param array $processed_form
   */
  private function processContent(&$content, &$processed_form) {
    $i = 0;
    while (isset($content[$i])) {
      $properties = $this->getFormBuilderProperties($content[$i]);
      if ($properties) {
        $parent['#form_builder'] = $properties;
        $this->processChildren($content[$i], $parent);
        $processed_form[] = $parent;
      }
      else {
        foreach ($content[$i] as $form) {
          if ($this->canBeProcessed($form)) {
            $this->processFormElement($form, $processed_form);
          }
        }
      }
      $i++;
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
   * Process form.
   *
   * @return array
   */
  private function processForm() {
    $processed_form = array();
    foreach ($this->form as $form) {
      if ($this->canBeProcessed($form)) {
        $this->processFormElement($form, $processed_form);
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
      if(!isset($branch['#value'])) {
        $i = 0;
        $i++;
        $break_here;
      }
      $value = $branch['#value'];
      $require_value = $properties['require_value'];
      $valid_value = is_string($value) && (!$require_value ? TRUE : trim($value) !== '');
      return !$valid_value;
    }
    else {
      $this->prune($branch['#children']);
      return count($branch['#children']) == 0 && $properties['require_value'];
    }
  }

}
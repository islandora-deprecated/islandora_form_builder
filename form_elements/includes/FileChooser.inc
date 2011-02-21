<?php

// $Id$

/**
 * @file
 *
 */

/**
 * 
 */
class FileChooser {
  
}

/**
 * Validate a filchooser form element.
 *
 * @param array $element
 *   Drupal filechooser form element.
 * @param string $form_state
 *   The state of the drupal form.
 */
function islandora_form_builder_elements_validate_filechooser($element, &$form_state) {
  module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
  $collection_policy = CollectionPolicy::loadFromCollection($form_state['storage']['collection_pid']);
  if ($collection_policy !== false) {
    $form_state['values'][$element['#name']] = $collection_policy->getStagingArea() . '/' . $element['#value'];
  }
}

/**
 * Processes a filechooser element.
 *
 * @param array $element
 *   Drupal filechooser form element.
 * @param string $edit
 *   Not used...
 * @param array $form_state
 *   The state of the drupal form.
 *
 * @return array
 *   Modified filechooser form element.
 */
function islandora_form_builder_elements_process_filechooser(&$element, $edit, &$form_state) {
  $element['#model_pid'] = $form_state['values']['models'];
  return $element;
}

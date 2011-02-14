<?php

// $Id$

/**
 * @file
 *
 */

function ife_filechooser_validate($element, &$form_state) {
  module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
  $cp = CollectionPolicy::loadFromCollection($form_state['storage']['collection_pid']);
  if ($cp !== false) {
    $form_state['values'][$element['#name']] = $cp->getStagingArea() . '/' . $element['#value'];
  }
}

function ife_filechooser_process($element, $edit, &$form_state) {
  $element['#model_pid'] = $form_state['values']['models'];

  return $element;
}

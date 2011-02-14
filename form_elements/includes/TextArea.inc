<?php

// $Id$

/**
 * @file
 *
 */
//-------------------------------------------------------------------------------
// ife_sticky_textarea_process
// Params:
//  element -
//  edit -
//  form_state -
// Returns:
//
// Our process callback to expand the control.
//-------------------------------------------------------------------------------
function ife_sticky_textarea_process($element, $edit, &$form_state) {
  if (isset($element['#sticky']) && $element['#sticky'] == TRUE) {
    $element['#type'] = 'sticky_textarea';
    $stickyName = preg_replace('/\]/', '', array_pop(preg_split('/\[/', $element['#name']))) . '-sticky';
    if (isset($element['#post'][$stickyName]) && strtolower($element['#post'][$stickyName]) == 'on') {
      $form_state['storage'][$element['#name']] = $element['#value'];
    }
    elseif (isset($form_state['storage'][$element['#name']])) {
      $element['#value'] = $form_state['storage'][$element['#name']];
      $element['#sticky_on'] = TRUE;
    }
  }
  return $element;
}

/**
 * Format a sticky textarea.
 *
 * @param $element
 *   An associative array containing the properties of the element.
 *   Properties used: title, value, description, rows, cols, required, attributes
 * @return
 *   A themed HTML string representing the textarea.
 *
 * @ingroup themeable
 */
function theme_sticky_textarea($element) {
  $class = array('form-textarea');


  // Add teaser behavior (must come before resizable)
  if (!empty($element['#teaser'])) {
    drupal_add_js('misc/teaser.js');
    // Note: arrays are merged in drupal_get_js().
    drupal_add_js(array('teaserCheckbox' => array($element['#id'] => $element['#teaser_checkbox'])), 'setting');
    drupal_add_js(array('teaser' => array($element['#id'] => $element['#teaser'])), 'setting');
    $class[] = 'teaser';
  }

  // Add resizable behavior
  if ($element['#resizable'] !== FALSE) {
    drupal_add_js('misc/textarea.js');
    $class[] = 'resizable';
  }

  $path = drupal_get_path('module', 'islandora_form_builder_elements');
  drupal_add_css($path . '/css/sticky.css');
  $stickyName = preg_replace('/\]/', '', array_pop(preg_split('/\[/', $element['#name']))) . '-sticky';
  $sticky = (isset($element['#sticky']) && $element['#sticky'] == TRUE ? '<input class="sticky" type="checkbox" name="' . $stickyName . '"' . (isset($element['#sticky_on']) && $element['#sticky_on'] == TRUE ? ' checked="checked"' : '') . '/><br/>' : '');


  _form_set_class($element, $class);
  return theme('form_element', $element, '<textarea cols="' . $element['#cols'] . '" rows="' . $element['#rows'] . '" name="' . $element['#name'] . '" id="' . $element['#id'] . '" ' . drupal_attributes($element['#attributes']) . '>' . check_plain($element['#value']) . '</textarea>' . $sticky);
}

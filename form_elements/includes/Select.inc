<?php

// $Id$

/**
 * @file
 *
 */

//-------------------------------------------------------------------------------
// ife_sticky_select_process
// Params:
//  element -
//  edit -
//  form_state -
// Returns:
//
// Our process callback to expand the control.
//-------------------------------------------------------------------------------
function ife_sticky_select_process($element, $edit, &$form_state) {
  if (isset($element['#sticky']) && $element['#sticky'] == TRUE) {
    $element['#type'] = 'sticky_select';
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
 * Format a sticky select.
 *
 * @param $element
 *   An associative array containing the properties of the element.
 *   Properties used: title, value, description, rows, cols, required, attributes
 * @return
 *   A themed HTML string representing the textarea.
 *
 * @ingroup themeable
 */
function theme_sticky_select($element) {
  $path = drupal_get_path('module', 'islandora_form_builder_elements');
  drupal_add_css($path . '/css/sticky.css');
  $stickyName = preg_replace('/\]/', '', array_pop(preg_split('/\[/', $element['#name']))) . '-sticky';
  $sticky = (isset($element['#sticky']) && $element['#sticky'] == TRUE ? '<input class="sticky" type="checkbox" name="' . $stickyName . '"' . (isset($element['#sticky_on']) && $element['#sticky_on'] == TRUE ? ' checked="checked"' : '') . '/><br/>' : '');


  $select = '';
  $size = $element['#size'] ? ' size="' . $element['#size'] . '"' : '';
  _form_set_class($element, array('form-select'));
  $multiple = $element['#multiple'];
  return theme('form_element', $element, '<select name="' . $element['#name'] . '' . ($multiple ? '[]' : '') . '"' . ($multiple ? ' multiple="multiple" ' : '') . drupal_attributes($element['#attributes']) . ' id="' . $element['#id'] . '" ' . $size . '>' . form_select_options($element) . '</select>' . $sticky);
}
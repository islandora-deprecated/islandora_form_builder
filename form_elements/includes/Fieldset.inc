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
  
  public static function process($element, $edit, &$form_state) {

  }
}
//-------------------------------------------------------------------------------
// ife_fieldset_process
// Params:
//  element -
//  edit -
//  form_state -
// Returns:
//
//-------------------------------------------------------------------------------
function ife_fieldset_process($element, $edit, &$form_state) {
  if (isset($element['#collapsed']) && ife_fieldset_children_has_value($element)) {
    unset($element['#collapsed']);
  }
  return $element;
}

//-------------------------------------------------------------------------------
// ife_fieldset_children_has_value
// Params:
//  element -
//  edit -
//  form_state -
// Returns:
//
//-------------------------------------------------------------------------------
function ife_fieldset_children_has_value($element) {
  $ret = false;
  $has_default_value = (isset($element['#default_value']) && !is_array($element['#default_value']) && trim($element['#default_value']) != '');
  $has_value = (isset($element['#value']) && !is_array($element['#value']) && trim($element['#value']) != '');
  if ($has_default_value || $has_value) {
    $ret = true;
  }
  else {
    foreach (element_children($element) as $key) {
      if (!$ret) {
        $ret = (isset($_POST[$key]) && (is_array($_POST[$key]) || trim($_POST[$key]) != '')) || ife_fieldset_children_has_value($element[$key]);
      }
    }
  }
  return $ret;
}
<?php

// $Id$

/**
 * @file
 *
 */

//-------------------------------------------------------------------------------
// ife_datepicker_process
// Params:
//  element -
//  edit -
//  form_state -
// Returns:
//
// Our process callback to expand the control.
//-------------------------------------------------------------------------------
function ife_datepicker_process($element, $edit, &$form_state) {
  if (isset($element['#sticky']) && $element['#sticky'] == TRUE) {
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
 * Theme function to format the output.
 *
 * We use the container-inline class so that all three of the HTML elements
 * are placed next to each other, rather than on separate lines.
 */
function theme_datepicker($element) {
  global $base_url;
  $path = drupal_get_path('module', 'islandora_form_builder_elements');
  drupal_add_js($path . '/js/jquery.ui.core.js');
  drupal_add_js($path . '/js/jquery.ui.widget.js');
  drupal_add_js($path . '/js/jquery.ui.datepicker.js');
  drupal_add_css($path . '/css/ui-lightness/jquery-ui-1.8.7.custom.css');
  $js = '$(document).ready(function () { $(\'#' . $element['#id'] . '\').datepicker({  buttonImage: \'' . $base_url . '/' . $path . '/images/date.png\', constrainInput: false, showOn: \'button\', changeMonth: true, changeYear: true }); });';

  $size = empty($element['#size']) ? '' : ' size="' . $element['#size'] . '"';
  $maxlength = empty($element['#maxlength']) ? '' : ' maxlength="' . $element['#maxlength'] . '"';
  $class = array('form-text', 'datepicker');
  $extra = '';
  $output = '<script type="text/javascript">' . $js . '</script>';
  $output.='<div class="container-inline mark">';

  if ($element['#autocomplete_path'] && menu_valid_path(array('link_path' => $element['#autocomplete_path']))) {
    drupal_add_js('misc/autocomplete.js');
    $class[] = 'form-autocomplete';
    $extra = '<input class="autocomplete" type="hidden" id="' . $element['#id'] . '-autocomplete" value="' . check_url(url($element['#autocomplete_path'], array('absolute' => TRUE))) . '" disabled="disabled" />';
  }
  _form_set_class($element, $class);

  if (isset($element['#field_prefix'])) {
    $output .= '<span class="field-prefix">' . $element['#field_prefix'] . '</span> ';
  }

  $output .= '<input type="text"' . $maxlength . ' name="' . $element['#name'] . '" id="' . $element['#id'] . '"' . $size . ' value="' . check_plain($element['#value']) . '"' . drupal_attributes($element['#attributes']) . ' />';

  if (isset($element['#field_suffix'])) {
    $output .= ' <span class="field-suffix">' . $element['#field_suffix'] . '</span>';
  }

  $path = drupal_get_path('module', 'islandora_form_builder_elements');
  drupal_add_css($path . '/css/sticky.css');
  $stickyName = preg_replace('/\]/', '', array_pop(preg_split('/\[/', $element['#name']))) . '-sticky';
  $sticky = (isset($element['#sticky']) && $element['#sticky'] == TRUE ? '<input class="sticky" type="checkbox" name="' . $stickyName . '"' . (isset($element['#sticky_on']) && $element['#sticky_on'] == TRUE ? ' checked="checked"' : '') . '/><br/>' : '');

  $output.='</div>';

  return theme('form_element', $element, $output . $sticky) . $extra;
}
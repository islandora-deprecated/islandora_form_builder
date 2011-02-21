<?php

// $Id$

/**
 * @file
 *
 */

/**
 *
 */
class Set {

  public static function populate(&$form, &$registered_nodes) {
    drupal_set_message('Date Picker Auto Populate has not been implemented yet.');
  }

}

//-------------------------------------------------------------------------------
// ife_list_process
// Params:
//  element -
//  edit -
//  form_state -
// Returns:
//
// Our process callback to expand the control.
//-------------------------------------------------------------------------------
function ife_list_process($element, $edit, &$form_state) {
  $element['#attributes'] = array('class' => 'list');
  if (isset($element['#default_value']) && is_array($element['#default_value'])) {
    $element['#default_value'] = join('; ', $element['#default_value']);
  }
  if (isset($element['#value']) && is_array($element['#value'])) {
    $element['#value'] = join('; ', $element['#value']);
  }
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

function theme_list($element) {
  $path = drupal_get_path('module', 'islandora_form_builder_elements');
  drupal_add_js($path . '/js/jquery.tag.editor-min.js');
  drupal_add_js($path . '/js/list.js');
  /*
    $js = "$(document).ready(function () {
    $('#".$element['#id']."').tagEditor( {
    separator: ';',
    confirmRemoval: false,
    completeOnBlur: true,
    });
    });";
    drupal_add_js($js, 'inline'); */
  drupal_add_css($path . '/css/list.css');


//return theme('theme_form_element', $element, '<div class="container-inline mark">' . $element['#children'] . '</div>');

  $size = empty($element['#size']) ? 'size="25"' : ' size="' . $element['#size'] . '"';
  $maxlength = empty($element['#maxlength']) ? '' : ' maxlength="' . $element['#maxlength'] . '"';
  $class = array('form-text', 'listElement');
  $extra = '';
  $output = '<div class="container-inline mark">';

  if (isset($element['#autocomplete_path']) && $element['#autocomplete_path'] && menu_valid_path(array('link_path' => $element['#autocomplete_path']))) {
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

  $name = preg_split('/\[/', $element['#name']);
  $stickyName = preg_replace('/\]/', '', array_pop($name)) . '-sticky';
  $sticky = (isset($element['#sticky']) && $element['#sticky'] == TRUE ? '<input class="sticky" type="checkbox" name="' . $stickyName . '"' . (isset($element['#sticky_on']) && $element['#sticky_on'] == TRUE ? ' checked="checked"' : '') . '/><br/>' : '');

  $output.='</div>';

  return theme('form_element', $element, $output . $sticky) . $extra;
}
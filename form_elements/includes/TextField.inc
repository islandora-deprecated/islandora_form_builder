<?php

// $Id$

/**
 * @file
 *
 */

/**
 *
 * @param <type> $element
 * @param <type> $edit
 * @param string $form_state
 * @param <type> $complete_form
 * @return boolean 
 */
function ife_sticky_textfield_process(&$element, $edit, &$form_state, $complete_form) {
  if (isset($element['#sticky']) && $element['#sticky'] == TRUE) {
    $element['#type'] = 'sticky_textfield';
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
function theme_sticky_textfield($element) {

  //echo '<pre>'; var_dump($element); echo '</pre>';
  //return theme('theme_form_element', $element, '<div class="container-inline mark">' . $element['#children'] . '</div>');
  $size = empty($element['#size']) ? '' : ' size="' . $element['#size'] . '"';
  $maxlength = empty($element['#maxlength']) ? '' : ' maxlength="' . $element['#maxlength'] . '"';
  $class = array('form-text');
  $extra = '';
  $output = '<div class="container-inline mark">';

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


  $output = $output . "</div>";
  return theme('form_element', $element, $output . $sticky) . $extra;
}

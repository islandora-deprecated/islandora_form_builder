<?php

// $Id$

/**
 * @file
 *
 */
class TextField {

  public static function populate(&$form, &$node_list) {
    module_load_include('inc', 'islandora_form_builder', 'FormPopulator');
    if ($node_list->length == 1) {
      $node = $node_list->item(0);
      $form['#default_value'] = $node->textContent;
      $form['#form_builder']['hash'] = FormPopulator::generateHash($node);
    }
  }

  public static function process(&$element, &$form_state) {
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

}

/**
 *
 * @param array $element
 * @param string $edit
 * @param array $form_state
 * @param array $complete_form
 * @return array
 */
function ife_sticky_textfield_process(&$element, $edit, &$form_state, $complete_form) {
  return TextField::process($element, $form_state);
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

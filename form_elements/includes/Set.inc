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

  /**
   * Populates the set, from an existing datastream.
   *
   * @param array $form
   *   Drupal set form element.
   * @param array $registered_nodes
   *   Nodes from an existing datastream that this set will repersent.
   */
  public static function populate(&$form, &$registered_nodes) {
    $i = 0;
    foreach ($registered_nodes as &$registered_node) {
      list($hash, $node) = $registered_node;
      $form['#form_builder']['hash'][] = $hash;
      $values[] = $node->textContent;
      $i++;
    }
    $form['#default_value'] = implode(';', $values);
  }

}

function islandora_form_builder_elements_ajax_set_remove($hash) {
  if (!isset($_POST['form_build_id']))
    return;
  $form_state = array('storage' => NULL, 'submitted' => FALSE, 'post' => $_POST);
  $form_build_id = $_POST['form_build_id'];
  if (!$form = form_get_cache($form_build_id, $form_state))
    exit();
  $path = $_POST['name'];
  $index = strpos($path, '[');
  $root = substr($path, 0, $index);
  $path = substr($path, $index);
  $command = "\$form_element = &\$form['$root']$path;";
  $command = str_replace('[content]', "['content']", $command);
  $form_element = '';
  $result = eval($command);
  $key = array_search($hash, $form_element['#form_builder']['hash']);
  unset($form_element['#form_builder']['hash'][$key]);
  // Save Changes
  $form_id = $form['#parameters'][0];
  $form['#post'] = $_POST;
  $form['#programmed'] = FALSE;
  form_set_cache($form_build_id, $form, $form_state);
}

/**
 * Process a set form element.
 * 
 * @param array $element
 *   Set form element.
 * @param string $edit
 *   Not used...
 * @param array $form_state
 *   The state of the drupal form.
 *
 * @return array
 *   The processed set form element.
 */
function islandora_form_builder_elements_process_set(&$element, $edit, &$form_state) {
  $element['#attributes'] = array('class' => 'list');
  if (isset($element['#default_value']) && is_array($element['#default_value'])) {
    $element['#default_value'] = join('; ', $element['#default_value']);
  }
  if (isset($element['#value']) && is_array($element['#value'])) {
    $element['#value'] = join('; ', $element['#value']);
  }
  return $element;
}

/**
 *
 * @param <type> $element
 * @return string
 *   HTML.
 */
function theme_set($element) {
  $path = drupal_get_path('module', 'islandora_form_builder_elements');
  drupal_add_js($path . '/js/jquery.tag.editor.js');
  drupal_add_js($path . '/js/list.js');
  drupal_add_css($path . '/css/list.css');

  $size = empty($element['#size']) ? 'size="25"' : ' size="' . $element['#size'] . '"';
  $max_length = empty($element['#maxlength']) ? '' : ' maxlength="' . $element['#maxlength'] . '"';
  $class = array('form-text', 'listElement');
  $extra = '';
  $output = '<div class="container-inline mark">';
  if(isset($element['#form_builder']) && isset($element['#form_builder']['hash'])) {
    $output .= '<div class="set-hash-tags" style="display:none">'. implode(';', $element['#form_builder']['hash']) .'</div>';
  }
  if (isset($element['#autocomplete_path']) && $element['#autocomplete_path'] && menu_valid_path(array('link_path' => $element['#autocomplete_path']))) {
    drupal_add_js('misc/autocomplete.js');
    $class[] = 'form-autocomplete';
    $extra = '<input class="autocomplete" type="hidden" id="' . $element['#id'] . '-autocomplete" value="' . check_url(url($element['#autocomplete_path'], array('absolute' => TRUE))) . '" disabled="disabled" />';
  }
  _form_set_class($element, $class);

  if (isset($element['#field_prefix'])) {
    $output .= '<span class="field-prefix">' . $element['#field_prefix'] . '</span> ';
  }

  $output .= '<input type="text"' . $max_length . ' name="' . $element['#name'] . '" id="' . $element['#id'] . '"' . $size . ' value="' . check_plain($element['#value']) . '"' . drupal_attributes($element['#attributes']) . ' />';

  if (isset($element['#field_suffix'])) {
    $output .= ' <span class="field-suffix">' . $element['#field_suffix'] . '</span>';
  }

  $output.='</div>';

  return theme('form_element', $element, $output) . $extra;
}
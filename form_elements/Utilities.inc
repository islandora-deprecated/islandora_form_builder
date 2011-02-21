<?php

// $Id$

/**
 * @file
 * 
 * Useful global functions.
 */

function islandora_form_builder_elements_load_all_include_files() {
  $include_path = drupal_get_path('module', 'islandora_form_builder_elements') . "/includes";
  $dir = opendir($include_path);
  if ($dir !== false) {
    while (($file = readdir($dir)) !== false) {
      if (preg_match('/\.inc$/', $file)) {
        require_once "$include_path/$file";
      }
    }
    closedir($dir);
  }
}

/**
 * Includes the proper file and returns the class name for that class.
 *
 * @param string $type
 *
 * @return string
 *   The class name for that type.
 */
function islandora_form_builder_elements_get_class_name_and_file_name_of_form_element_type($type) {
  global $form_element_class_and_file;
  if (isset($form_element_class_and_file[$type])) {
    return $form_element_class_and_file[$type];
  }
  $include_path = drupal_get_path('module', 'islandora_form_builder_elements') . "/includes";
  $dir = opendir($include_path);
  if ($dir !== false) {
    while (($file = readdir($dir)) !== false) {
      if (preg_match('/\.inc$/', $file)) {
        $tokens = token_get_all(file_get_contents("$include_path/$file"));
        $class_token = false;
        foreach ($tokens as $token) {
          if (!is_array($token))
            continue;
          if ($token[0] == T_CLASS) {
            $class_token = true;
          }
          else if ($class_token && $token[0] == T_STRING) {
            if (strcasecmp($token[1], $type) == 0) {
              closedir($dir);
              $form_element_class_and_file[$type] = array($token[1], "$include_path/$file");
              return $form_element_class_and_file[$type];
            }
            $class_token = false;
          }
        }
      }
    }
    closedir($dir);
  }
  return FALSE;
}

/**
 * @file
 *
 */
function islandora_form_builder_elements_get_allowed_form_elements() {
  // Load Islandora Content Model Schema
  $filename = drupal_get_path('module', 'fedora_repository') . '/islandoracm.xsd';
  $schema_document = new DOMDocument();
  $schema_document->load($filename);
  $xpath = new DOMXPath($schema_document);
  $xpath->registerNamespace('xsd', "http://www.w3.org/2001/XMLSchema");
  // Find all Element types that can be used in Form Generation
  $result = $xpath->query('//xsd:complexType[@name=\'formElementType\']/xsd:choice/xsd:element/@name');
  $types = array();
  for ($i = 0; $i < $result->length; $i++) {
    $node = $result->item($i);
    $types[] = $node->value;
  }
  return $types;
}
<?php

// $Id$

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
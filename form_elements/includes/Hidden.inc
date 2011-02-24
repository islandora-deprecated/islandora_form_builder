<?php

// $Id$

/**
 * @file
 *
 */
class Hidden {

  /**
   * Creates a number of panels based on how many nodes exist.
   *
   * @param array $form
   * @param DOMNodeList $node_list
   */
  static public function populate(&$form, &$registered_nodes) {
    if (isset($form['content'])) {
      $i = 0;
      foreach ($registered_nodes as &$registered_node) {
        list($hash, $node) = $registered_node;
        $form['content'][$i] = $form['content'][0];
        $form['content'][$i]['#form_builder'] = $form['#form_builder'];
        $xpath_index = $i + 1;
        $form['content'][$i]['#form_builder']['path'][0] .= "[$xpath_index]"; // XPath indices start at 1
        $form['content'][$i]['#form_builder']['hash'] = $hash;
        $i++;
      }
    }
    else {
      list($hash, $node) = $registered_nodes[0];
      $form['#default_value'] = trim($node->textContent);
      $form['#form_builder']['hash'] = $hash;
    }
  }

}
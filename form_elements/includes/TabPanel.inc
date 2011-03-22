<?php

// $Id$

/* @file
 *
 *  Allows forms to dynamically add new tabs to a tabpanel in a form.
 */


class TabPanel {

  /**
   * Creates a number of panels based on how many nodes exist.
   *
   * @param array $form
   * @param DOMNodeList $node_list
   */
  static public function populate(&$form, &$registered_nodes) {
    $i = 0;
    foreach ($registered_nodes as &$registered_node) {
      list($hash, $node) = $registered_node;
      $form['content'][$i] = $form['content'][0];
      $form['content'][$i]['#form_builder'] = $form['#form_builder'];
      $form['content'][$i]['#form_builder']['hash'] = $hash;
      $i++;
    }
  }

  public function theme(&$element) {
    global $base_url;
    $path = drupal_get_path('module', 'islandora_form_builder_elements');
    drupal_add_js($path . '/js/jquery-ui-1.8.7.custom.min.js');
    drupal_add_js($path . '/js/tabpanel_ahah.js');
    drupal_add_css($path . '/css/ui-lightness/jquery-ui-1.8.7.custom.css');
    $classes = 'action-load-tabpanel';
    if($element['#collapsible']) {
      $classes .= ' property-collapsible-tabpanel';
    }
    if($element['#collapsed']) {
      $classes .= ' property-collapsed-tabpanel';
    }
    $id = t(implode('-', $element['#array_parents']));
    return "<div id='$id' class='$classes'>{$element['#children']}</div>";
  }

  public function themeEachTab(&$form) {
    $name = $form['#name'];
    $title = $form['#title'];
    $output = $this->themeTabHeader($form, $name, $title);
    $output .= $this->themeTabContent($form, $name, $title);
    return $output;
  }

  private function themeTabHeader(&$form, $name, $title) {
    $output = '<ul>';
    $output .= '<li>';
    $output .= "<a href='#$name-0'>$title</a><span id='add-$name' class='ui-icon ui-icon-circle-plus' style='float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;'>";
    $output .= $form[0]["add-tab"]['#id'];
    $output .= '</span>';
    $output .= '</li>';
    foreach ($form as $key => $panel) {
      if (is_numeric($key) && $key > 0) {
        $output .= '<li>';
        $output .= "<a href='#$name-$key'>$key</a>";
        $output .= "<span id='remove-$name-$key' class='ui-icon ui-icon-close' style='float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;'>";
        $output .= $panel['remove-tab']['#id'];
        $output .= '</span>';
        $output .= '</li>';
      }
    }
    $output .= '</ul>';
    return $output;
  }

  private function themeTabContent(&$form, $name, $title) {
    $output = '';
    foreach ($form as $key => $panel) {
      if (is_numeric($key)) {
        $output .= "<div id='$name-$key'>";
        $output .= drupal_render($form[$key]);
        $output .= '</div>';
      }
    }
    return $output;
  }

  public function process(&$form, &$form_state) {
    $form['#tree'] = TRUE;
    $wrapper = $form['#id'] . '-wrapper';
    $this->wrap($form, $wrapper);
    $this->addThemeInfoToContent($form['content'], $form['#title'], $form['#id']);
    $path = $form['#parents'];
    array_walk($path, array($this, 'stringifyPath'));
    $path = implode('', $path);
    $this->createAddTabButton($form['content'], $path, $wrapper);
    $this->createRemoveTabButtons($form['content'], $path, $wrapper);
    return $form;
  }

  private function stringifyPath(&$value, $key) {
    if (is_numeric($value)) {
      $value = "[$value]";
    }
    else {
      $value = "['$value']";
    }
  }

  /**
   * Wraps the tabpanel in a div element, this is used by ahah.
   *
   * @param array $form
   * @param string $wrapper
   */
  private function wrap(&$form, $wrapper) {
    $form['#prefix'] = '<div class="clear-block" id="' . $wrapper . '">';
    $form['#suffix'] = '</div>';
  }

  private function addThemeInfoToContent(&$content, $title, $id) {
    $content['#title'] = $title;
    $content['#theme'] = 'tabpanel_items';
    $content['#name'] = $id;
  }

  private function createAddTabButton(&$content, $path, $wrapper) {
    $content[0]['add-tab'] = array(
      '#type' => 'submit',
      '#weight' => 4,
      '#size' => 30,
      '#attributes' => array('style' => 'display:none'), // Hidden, triggered via javascript.
      '#value' => "add $path",
      '#ahah' => array(
        'path' => 'formbuilder/elements/ahah/tabpanel',
        'wrapper' => $wrapper,
        'method' => 'replace',
        'effect' => 'fade'
      )
    );
  }

  private function createRemoveTabButtons(&$content, $path, $wrapper) {
    foreach ($content as $key => &$panel) {
      if (is_numeric($key) && $key > 0) {
        $panel['remove-tab'] = array(
          '#type' => 'submit',
          '#weight' => 4,
          '#size' => 30,
          '#attributes' => array('style' => 'display:none'), // Hidden, triggered via javascript.
          '#value' => "remove $key $path", // Unique
          '#ahah' => array(
            'path' => 'formbuilder/elements/ahah/tabpanel',
            'wrapper' => $wrapper,
            'method' => 'replace',
            'effect' => 'fade'
          ),
        );
      }
    }
  }

  public function ahah($form_build_id) {
    list($form, $form_state) = $this->getFormAndState($form_build_id);
    $path = $this->performAction($form, $form_state);
    $this->saveChanges($form_build_id, $form, $form_state);
    $form = $this->processForm($form, $form_state);
    $tabpanel = $this->findTabPanel($form, $path);
    $this->sendJSON($tabpanel);
  }

  private function getFormAndState($form_build_id) {
    $form_state = array('storage' => NULL, 'submitted' => FALSE, 'post' => $_POST);
    if (!$form = form_get_cache($form_build_id, $form_state))
      exit();
    return array(&$form, &$form_state);
  }

  private function performAction(&$form, &$form_state) {
    $params = explode(' ', $form_state['post']['op']);
    $action = array_shift($params);
    switch ($action) {
      case 'add':
        $path = array_shift($params);
        $tabpanel = &$this->findTabPanel($form, $path);
        $this->addPanel($tabpanel);
        break;
      case 'remove':
        list($index, $path) = $params;
        $tabpanel = &$this->findTabPanel($form, $path);
        $post = &$this->findPost($form, $path);
        $this->removePanel($index, $tabpanel, $post);
        break;
    }
    return $path; // $path of the modified tabpanel.
  }

  private function addPanel(&$tabpanel) {
    $panel = $tabpanel['content'][0]; // Copy the first panel.
    $tabpanel['content'][] = &$panel; // Add it to the tabpanel
    $this->pruneExtraPanels($panel); // Remove extra tabpanels.
    $this->removeCopiedHashes($panel);
    $this->removeDefaultValues($panel);
  }

  private function pruneExtraPanels(&$panel) {
    foreach ($panel as $key => &$child) {
      if (is_numeric($key) && isset($child['content'])) {
        $content = &$child['content'];
        $this->removeExtraPanels($content);
        $this->pruneExtraPanels($content[0]);
      }
    }
  }

  private function removeExtraPanels(&$content) {
    foreach ($content as $key => &$panel) {
      if (is_numeric($key) && $key > 0) {
        unset($content[$key]);
      }
    }
  }

  private function removeCopiedHashes(&$panel) {
    $this->removeHash($panel);
    foreach ($panel as $key => &$child) {
      if (is_numeric($key)) {
        $this->removeHash($child);
        if (isset($child['content'])) {
          $this->removeCopiedHashes($child['content'][0]);
        }
      }
    }
  }

  private function removeHash(&$form) {
    if (isset($form['#form_builder'])) {
      if (isset($form['#form_builder']['hash'])) {
        unset($form['#form_builder']['hash']);
      }
    }
  }

  private function removeDefaultValues(&$panel) {
    $this->removeHash($panel);
    foreach ($panel as $key => &$child) {
      if (is_numeric($key)) {
        $this->removeDefaultValue($child);
        if (isset($child['content'])) {
          $this->removeDefaultValues($child['content'][0]);
        }
      }
    }
  }

  private function removeDefaultValue(&$form) {
    if (isset($form['#default_value'])) {
      unset($form['#default_value']);
    }
  }

  private function removePanel($index, &$tabpanel, &$post) {
    array_splice($tabpanel['content'], $index, 1);
    array_splice($post['content'], $index, 1);
  }

  private function saveChanges(&$form_build_id, &$form, &$form_state) {
    $form['#post'] = $_POST;
    $form['#programmed'] = FALSE;
    form_set_cache($form_build_id, $form, $form_state);
  }

  private function processForm(&$form, &$form_state) {
    $form_id = $form['#parameters'][0];
    return form_builder($form_id, $form, $form_state);
  }

  private function &findTabPanel(&$form, &$path) {
    $tabpanel = '';
    $command = "\$tabpanel = &\$form$path;";
    $result = eval($command);
    return $tabpanel;
  }

  private function &findPost(&$form, &$path) {
    $post = '';
    $command = "\$post = &\$_POST$path;";
    $result = eval($command);
    return $post;
  }

  private function sendJSON(&$tabpanel) {
    unset($tabpanel['#prefix'], $tabpanel['#suffix']);
    $javascript = drupal_add_js(NULL, NULL, 'header');
    $settings = call_user_func_array('array_merge_recursive', $javascript['setting']);
    unset($settings['ahah']['']);
    drupal_json(array(
      'status' => TRUE,
      'data' => theme('status_messages') . drupal_render($tabpanel),
      'settings' => $settings,
    ));
  }

}

/**
 * Menu hook for ahah callbacks.
 *
 * Depending on the button clicked this will remove a panel or add one.
 */
function islandora_form_builder_elements_ahah_tabpanel() {
  if (!isset($_POST['form_build_id']))
    return;
  $tab_panel = new TabPanel();
  $tab_panel->ahah($_POST['form_build_id']);
}

/**
 * Expands the TabPanel form element.
 *
 * @param array $form
 *   Drupal tabpanel form element.
 * @param string $edit
 *   Not used.
 * @param array $form_state
 *   The state of the drupal form.
 *
 * @return array
 *   Modified drupal tabpanel form element.
 */
function islandora_form_builder_elements_process_tabpanel(&$form, $edit, &$form_state) {
  $tab_panel = new TabPanel();
  return $tab_panel->process($form, $form_state);
}

/**
 * Theme hook for a tabpanel
 */
function theme_tabpanel($element) {
  $tab_panel = new TabPanel();
  return $tab_panel->theme($element);
}

/**
 * Theme hook for the tabpanels contents.
 */
function theme_tabpanel_items($form) {
  $tab_panel = new TabPanel();
  return $tab_panel->themeEachTab($form);
}

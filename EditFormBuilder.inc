<?php

// $Id$

/*
 * @file
 *
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');
module_load_include('inc', 'islandora_form_builder', 'FormBuilderInterface');

/**
 *
 */
class IngestFormBuilder implements FormBuilderInterface {

  /**
   * Drupal persistance form storage, specifically the index STORAGE_KEY where all
   * persistant data related to the islandora_form_builder is kept.
   *
   * @var array
   */
  protected $storage;
  /**
   * The pid of the object whose datastream will be modify.
   *
   * @var string
   */
  protected $pid;
  /**
   * The datatstrean ID for the datastream we will modify.
   *
   * @var string
   */
  protected $dsid;

  /**
   * Create the Edit Form Builder.
   *
   * @param array $form_state
   * @param string $pid
   * @param string $dsid
   */
  public function __construct(&$form_state, $pid = NULL, $dsid = NULL) {
    $this->initializeStorage($form_state, $pid, $dsid);
    $this->formState = &$form_state;
    $this->storage = $this->formState['storage'][STORAGE_KEY];
    $this->pid = $this->storage['pid'];
    $this->dsid = $this->storage['dsid'];
  }

  /**
   * Initialize the form's persistant storage.
   *
   * @param array $form_state
   * @param string $pid
   * @param string $dsid
   */
  private function initializeStorage(&$form_state, $pid = NULL, $dsid = NULL) {
    if (is_array($form_state['storage'][STORAGE_KEY])) {
      $form_state['storage'][STORAGE_KEY] = array();
    }
    $storage = &$form_state['storage'][STORAGE_KEY];
    $storage['pid'] = is_string($storage['pid']) ? $storage['pid'] : $pid;
    $storage['dsid'] = is_string($storage['dsid']) ? $storage['dsid'] : $dsid;
  }

  /**
   * Gets the edit form for rendering.
   *
   * @return array
   *   Returns a Drupal form if the user can create one and the form exists in the content model,
   *   otherwise it returns FALSE.
   */
  public function createForm() {
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    $content_model = ContentModel::loadFromObject($this->pid);
    if ($content_model !== FALSE) {
      return $content_model->buildEditMetadataForm($this->pid, $this->dsid); // TODO handle this without the content model
    }
  }

  /**
   *
   * @param array $form
   *   Drupal form to be validated.
   */
  public function validateForm(&$form) {

  }

  /**
   *
   * @param array $form
   *   Drupal form to be validated.
   */
  public function submitForm(&$form) {
    module_load_include('inc', 'fedora_repository', 'ConnectionHelper');
    global $base_url;
    if (strstr($form_state['clicked_button']['#id'], 'edit-submit')) {

      //$client = getSoapClient(variable_get('fedora_soap_manage_url', 'http://localhost:8080/fedora/services/management?wsdl'));
      $soap_helper = new ConnectionHelper();
      $client = $soap_helper->getSoapClient(variable_get('fedora_soap_manage_url', 'http://localhost:8080/fedora/services/management?wsdl'));

      // Check the content model for a custom edit metadata form submit function.
      if (isset($form_state['values']['pid'])) {
        module_load_include('inc', 'fedora_repository', 'ContentModel');
        if (($cm = ContentModel::loadFromObject($form_state['values']['pid'])) !== FALSE) {
          return $cm->handleEditMetadataForm($form_state['values']['form_id'], $form_state, $client);
        }
      }

      module_load_include('inc', 'fedora_repository', 'formClass');
      $metaDataForm = new formClass();
      $return_value = $metaDataForm->updateMetaData($form_state['values']['form_id'], $form_state['values'], $client);
      $form_state['redirect'] = $base_url . '/fedora/repository/' . $form_state['values']['pid'];

      return $return_value;
    }
  }

}
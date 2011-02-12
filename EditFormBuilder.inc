<?php

// $Id$

/*
 * @file
 *
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');

/**
 *
 */
class IngestFormBuilder {

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
   * Gets the ingest form for rendering.
   *
   * @return array
   *   Returns a Drupal form if the user can create one and the form exists in the content modeler,
   *   otherwise it returns FALSE.
   */
  public function createForm() {
    global $user;
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    module_load_include('inc', 'fedora_repository', 'ObjectHelper');

    

    //module_load_include('inc', 'fedora_repository', 'formClass');
    module_load_include('inc', 'fedora_repository', 'ConnectionHelper');
    $soapHelper = new ConnectionHelper();

    $client = $soapHelper->getSoapClient(variable_get('fedora_soap_url', 'http://localhost:8080/fedora/services/access?wsdl'));
    // Check if there is a custom edit metadata function defined in the content model.

    $breadcrumbs = array();
    $objectHelper = new ObjectHelper();
    $objectHelper->getBreadcrumbs($this->pid, $breadcrumbs);
    drupal_set_breadcrumb(array_reverse($breadcrumbs));

    $output = '';
    if (($content_model = ContentModel::loadFromObject($this->pid)) !== FALSE) {
      $output = $content_model->buildEditMetadataForm($this->pid, $this->dsid);
    }

    if (empty($output)) {
      // There is no custom function, so just load the standard QDC form.
      $metaDataForm = new formClass();
      //currently we only edit the dc metadata.  If you defined a custom form with a custom handler you are sol for now.
      return $metaDataForm->createMetaDataForm($this->pid, $this->dsid, $client);
    }
    return $output;
  }

}
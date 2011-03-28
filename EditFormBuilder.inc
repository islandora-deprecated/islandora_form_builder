<?php

// $Id$

/*
 * @file
 *
 */
module_load_include('inc', 'islandora_form_builder', 'FormBuilderInterface');

/**
 *
 */
class EditFormBuilder implements FormBuilderInterface {

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
   * The name of the form to create.
   *
   * @var string
   */
  protected $formName;

  /**
   * Create the Edit Form Builder.
   *
   * @param array $form_state
   * @param string $pid
   */
  public function __construct(&$form_state, $pid = NULL, $form_name = NULL) {
    $this->initializeStorage($form_state, $pid, $form_name);
    $this->formState = &$form_state;
    $this->storage = &$this->formState['storage'][STORAGE_KEY];
    $this->pid = $this->storage['pid'];
    $this->formName = $this->storage['form_name'];
  }

  /**
   * Initialize the form's persistant storage.
   *
   * @param array $form_state
   * @param string $pid
   */
  private function initializeStorage(&$form_state, $pid = NULL, $form_name = NULL) {
    if (!is_array($form_state['storage'][STORAGE_KEY])) {
      $form_state['storage'][STORAGE_KEY] = array();
    }
    $storage = &$form_state['storage'][STORAGE_KEY];
    $storage['pid'] = isset($storage['pid']) ? $storage['pid'] : $pid;
    $storage['form_name'] = isset($storage['form_name']) ? $storage['form_name'] : $form_name;
  }

  /**
   * Gets the edit form for rendering.
   *
   * @return array
   *   Returns a Drupal form if the user can create one and the form exists in the content model,
   *   otherwise it returns FALSE.
   */
  public function createForm() {
    $builder = &$this->createEditFormBuilder();
    return $builder->createForm($form);
  }

  /**
   * Not currently used...
   *
   * @param array $form
   *   Drupal form to be validated.
   */
  public function validateForm(&$form) {
    $builder = &$this->createEditFormBuilder();
    return $builder->validateForm($form);
  }

  /**
   * Submit the form. Modify the datastream identified by $this->dsid.
   *
   * @param array $form
   *   Drupal form to be validated.
   */
  public function submitForm(&$form) {
    $builder = &$this->createEditFormBuilder();
    return $builder->submitForm($form);
  }

  private function &createEditFormBuilder() {
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    $content_model = ContentModel::loadFromObject($this->pid);
    $form_element = $content_model->getForm($this->formName);
    $module = $form_element->getAttribute('edit_module');
    $filename = $form_element->getAttribute('edit_file');
    $class = $form_element->getAttribute('edit_class');
    $dsid = $form_element->getAttribute('dsid');
    $file = './' . drupal_get_path('module', $module) . "/$filename";
    if (is_file($file)) {
      require_once $file;
    }
    else {
      return FALSE;
    }
    $form = new $class($this->formState, $this->pid, $dsid,  $this->formName);
    return $form;
  }

}
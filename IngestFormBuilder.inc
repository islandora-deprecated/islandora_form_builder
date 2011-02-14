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
   * The pid of the collection that an object will be ingest into. Or the
   * collection in which the object to be edited belongs to.
   *
   * @var string
   */
  protected $collectionPid;
  /**
   * The label for the collection identified by $collectionPid. Really only used to show the users what
   * collection they are dealing with.
   *
   * @var string
   */
  protected $collectionLabel;
  /**
   * The current page of this multi-page form.
   *
   * @var integer
   */
  protected $page;
  /**
   * Drupal form state
   *
   * @var array
   */
  protected $formState;

  /**
   * Create the Ingest Form Builder.
   *
   * @param array $form_state
   * @param string $collection_pid
   * @param string $collection_label
   */
  public function __construct(&$form_state, $collection_pid = NULL, $collection_label = NULL) {
    $this->initializeStorage($form_state, $collection_pid, $collection_label);
    $this->formState = &$form_state;
    $this->storage = &$this->formState['storage'][STORAGE_KEY];
    $this->collectionPid = $this->storage['collection_pid'];
    $this->collectionLabel = $this->storage['collection_label'];
    $this->page = &$this->storage['ingest_form_page'];
  }

  /**
   * Initialize the form's persistant storage.
   *
   * @param array $form_state
   * @param string $collection_pid
   * @param string $collection_label
   */
  private function initializeStorage(&$form_state, $collection_pid = NULL, $collection_label = NULL) {
    if (!isset($form_state['storage'][STORAGE_KEY]) || !is_array($form_state['storage'][STORAGE_KEY])) {
      $form_state['storage'][STORAGE_KEY] = array();
    }
    $storage = &$form_state['storage'][STORAGE_KEY];
    $storage['collection_pid'] = isset($storage['collection_pid']) ? $storage['collection_pid'] : $collection_pid;
    $storage['collection_label'] = isset($storage['collection_label']) ? $storage['collection_label'] : $collection_label;
    if (empty($storage['ingest_form_page'])) { // First page by default.
      $storage['ingest_form_page'] = 1;
    }
  }

  /**
   * Gets the ingest form for rendering.
   *
   * @return array
   *   Returns a Drupal form if the user is allowed to ingest and the form exists in the content model,
   *   otherwise it returns FALSE.
   */
  public function createForm() {
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    if (!$this->canIngest()) {
      return FALSE;
    }
    return $this->getForm();
  }

  /**
   * Tests to see if the user can ingest.
   *
   * @return boolean
   *   TRUE if the user can ingest FALSE otherwise.
   */
  private function canIngest() {
    if (!user_access('ingest new fedora objects')) { // Drupal Security
      drupal_set_message(t('You do not have permission to ingest.'), 'error');
      return FALSE;
    }
    else if (!$this->canIngestObjectIntoCollection($this->collectionPid)) { // Fedora security
      drupal_set_message(t('You do not have premission to ingest here.'));
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Tests if the user can ingest into the collection specified by $collection_pid
   *
   * Queries the collection object for a childsecurity datastream and if found parses it
   * to determine if this user is allowed to ingest in this collection we assume if
   * they are able to modify objects in the collection they can ingest as well.
   *
   * @param string $collection_pid
   *   The pid of the collection that will be queried to see if ingest is possible.
   *
   * @return boolean
   *   True if the user can ingest an object into the collection,
   *   specified by $collection_pid
   */
  private function canIngestObjectIntoCollection($collection_pid) {
    module_load_include('inc', 'fedora_repository', 'SecurityClass');
    $securityClass = new SecurityClass();
    return $securityClass->canIngestHere($collection_pid);
  }

  /**
   * Gets the appropriate ingest form, based on the forms page number.
   *
   * @return array
   *   The Drupal form to be rendered.
   */
  private function getForm() {
    return $this->page == 1 ?
        $this->getFormPageOne() :
        $this->getFormPageTwo();
  }

  /**
   * Gets the first page of the ingest form.
   *
   * The first page of the ingest form allows the user to select the Content Model, they want the ingest object
   * to have. It also allows the user to select which ingest form they want to use, if more than one exist. Optionally
   * in the Content modeler a particular form may be forced in that case the other forms will not be listed as options.
   *
   * @return array
   *   The first page of the ingest form.
   */
  private function getFormPageOne() {
    $collection_policy = CollectionPolicy::loadFromCollection($this->collectionPid);
    if ($collection_policy === FALSE) {
      drupal_set_message(t('Unable to load collection policy \'' . $this->collectionPid . '\'.'), 'error');
      return FALSE;
    }
    $content_models = $collection_policy->getContentModels();
    if (!$content_models) {
      $error_msg = 'No content models associated with this collection: !collection_label. Please contact your administrator.';
      drupal_set_message(t($error_msg, array('!collection_label' => $this->collectionLabel)), 'error');
      return FALSE;
    }
    $models_for_form = array();
    foreach ($content_models as $content_model) {
      $identifier = $content_model->getIdentifier();
      $name = $content_model->name;
      $models_for_form["$identifier"] = "$name";
    }
    $form['indicator'] = array(
      '#type' => 'fieldset',
      '#title' => t('Ingest digital object into collection_pid !collection_label Step #1',
          array('collection_pid' => $this->collectionPid, '!collection_label' => $this->collectionLabel))
    );
    $form['indicator']['models'] = array(// Content models available
      '#type' => 'select',
      '#title' => t('Content models available'),
      '#options' => $models_for_form,
      '#description' => t('Content models define datastream composition, relationships between this and other ' .
          'content models, and the mandatory behaviors associated with each digital object.<br /> Additional ' .
          'information may be found ' .
          '<a href="https://wiki.duraspace.org/display/FEDORACREATE/Content+Models+Overview">here.</a> ')
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Next')
    );
    return $form;
  }

  /**
   *
   */
  private function getFormPageTwo() {
    module_load_include('inc', 'islandora_form_builder', 'FormGenerator');
    $form_values = $this->formState['values'];
    $content_model_pid = ContentModel::getPidFromIdentifier($form_values['models']);
    $content_model_dsid = ContentModel::getDSIDFromIdentifier($form_values['models']);
    $form_generator = new FormGenerator($content_model_pid, $content_model_dsid);
    $form = $form_generator->generateForm('full');
    return $form;
    var_dump($form);
    exit();

    $content_model = ContentModel::loadFromModel($content_model_pid, $content_model_dsid);

    //
    if ($content_model !== FALSE) {
      $form = array();
      $form = $content_model->buildIngestForm($form, $this->formState); // TODO handle this without the content model.
      if ($form === FALSE) {
        drupal_set_message(t("Error Building Ingest Form."), 'error');
        foreach (ContentModel::$errors as $error) {
          drupal_set_message($error, 'error');
        }
      }
    }
    
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Ingest'),
      '#prefix' => t('Please be patient. Once you click next there may be a number of files created. ' .
          'Depending on your content model this could take a few minutes to process.<br />')
    );
    return $form;
  }

  /**
   * Validates the ingest form based on the forms current page number.
   *
   * @param array
   *   Drupal form to be validated.
   */
  public function validateForm(&$form) {
    $this->page == 1 ?
            $this->validateFormOne() :
            $this->validateFormTwo($form);
  }

  /**
   * Changes the form to page to and rebuilds.
   */
  private function validateFormOne() {
    $this->page++;
    $this->formState['rebuild'] = TRUE;
  }

  /**
   * Validate the second page of the ingest form. This should really be refactored out such that a the user
   * of this class can specifiy thier own validator.
   *
   * @param array $form
   *   Drupal form to be validated.
   */
  private function validateFormTwo(&$form) {
    $validators = array();
    $form_values = &$this->formState['values'];

    if (!empty($_FILES['files']['name']['ingest-file-location'])) {
      $file_object = file_save_upload('ingest-file-location', $validators);
      file_move($file_object->filepath, 0, 'FILE_EXISTS_RENAME');
      $form_values['ingest-file-location'] = $file_object->filepath;
    }

    if (!file_exists($form_values['ingest-file-location'])) {
      $this->formState['rebuild'] = FALSE;
      return;
    }
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    module_load_include('inc', 'fedora_repository', 'MimeClass');
    $file = $form_values['ingest-file-location'];

    $content_model_pid = ContentModel::getPidFromIdentifier($form_values['models']);
    $content_model_dsid = ContentModel::getDSIDFromIdentifier($form_values['models']);
    $content_model = ContentModel::loadFromModel($content_model_pid, $content_model_dsid);
    if ($content_model === FALSE) {
      $this->formState['rebuild'] = FALSE;
      return;
    }

    $allowed_mime_types = $content_model->getMimetypes();
    $mime = new MimeClass();
    $mime_type = $mime->getType($file);

    if (!empty($file)) {
      if (!in_array($mime_type, $allowed_mime_types)) {
        $error = t("The uploaded file\'s mimetype ($mime_type) is not associated with this Content Model. The allowed types are " .
                implode(' ', $allowed_mime_types));
        form_set_error('ingest-file-location', $error);
        file_delete($file);
        return;
      }
      else if (!$content_model->execIngestRules($file, $mime_type)) {
        drupal_set_message(t('Error following Content Model Rules'), 'error');
        foreach (ContentModel::$errors as $error) {
          drupal_set_message($error, 'error');
        }
      }
    }
  }

  /**
   *
   * @param array
   *  $form
   */
  public function submitForm(&$form) {
    global $base_url;
    global $user;
    module_load_include('inc', 'fedora_repository', 'CollectionClass');
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    module_load_include('inc', 'fedora_repository', 'ContentModel');

    $form_values = &$this->formState['values'];
    $content_model_pid = ContentModel::getPidFromIdentifier($form_values['models']);
    $content_model_dsid = ContentModel::getDSIDFromIdentifier($form_values['models']);
    $error = TRUE;
    $should_redirect = TRUE;
    $redirect_to = $base_url . "/fedora/repository/{$form_values['collection_pid']}";
    $collection_policy = CollectionPolicy::loadFromCollection($form_values['collection_pid']);
    if ($collection_policy === FALSE) {
      $this->formState['redirect'] = ($error) ? ' ' : $redirect_to;
      return;
    }

    $relationship = $collection_policy->getRelationship();
    $content_model = ContentModel::loadFromModel($content_model_pid, $content_model_dsid);
    if ($content_model === FALSE) {
      $this->formState['redirect'] = ($error) ? ' ' : $redirect_to;
      return;
    }
    $pid = $collection_policy->getNextPid($content_model_dsid);
    $form_values['user_id'] = $user->name;
    $form_values['pid'] = $pid;
    $form_values['content_model_pid'] = $content_model_pid;
    $form_values['relationship'] = $relationship;
    $error = (!$content_model->execFormHandler($form_values));
    $_SESSION['fedora_ingest_files'] = ''; //empty this variable
    $attributes = $content_model->getIngestFormAttributes();
    $should_redirect = $attributes['redirect'];

    if ($should_redirect) {
      $this->formState['storage'] = NULL;
      $this->formState['redirect'] = ($error) ? ' ' : $redirect_to;
    }
  }

}
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
class ModsIngestFormBuilder implements FormBuilderInterface {

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
   * The pid of the content model that will be used to create the object. This is
   * only availible after the first form has been validated.
   *
   * @var string
   */
  protected $contentModelPid;
  /**
   * The ID of the XML datastream that the second form repersents. This is
   * only availible after the first form has been validated.
   *
   * @var string
   */
  protected $contentModelDsid;
  /**
   * The label for the collection identified by $collectionPid. Really only used to show the users what
   * collection they are dealing with.
   *
   * @var string
   */
  protected $collectionLabel;
  /**
   * The name of the ingest form to be build. This gets set on the first page of the form.
   *
   * @var string
   */
  protected $formName;
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
  public function __construct(&$form_state) {
    $this->initializeStorage($form_state);
    $this->formState = &$form_state;
    $this->storage = &$this->formState['storage'][STORAGE_KEY];
    $this->collectionPid = $this->storage['collection_pid'];
    $this->collectionLabel = $this->storage['collection_label'];
    $this->contentModelPid = isset($this->storage['content_model_pid']) ? $this->storage['content_model_pid'] : NULL;
    $this->contentModelDsid = isset($this->storage['content_model_dsid']) ? $this->storage['content_model_dsid'] : NULL;
    $this->formName = isset($this->storage['form_name']) ? $this->storage['form_name'] : NULL;
    $this->page = &$this->storage['ingest_form_page'];
  }

  /**
   * Initialize the form's persistant storage.
   *
   * @param array $form_state
   * @param string $collection_pid
   * @param string $collection_label
   */
  private function initializeStorage(&$form_state) {
    if (!isset($form_state['storage'][STORAGE_KEY]) || !is_array($form_state['storage'][STORAGE_KEY])) {
      $form_state['storage'][STORAGE_KEY] = array();
    }
    $storage = &$form_state['storage'][STORAGE_KEY];
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
     module_load_include('inc', 'islandora_form_builder', 'FormGenerator');
    $form_values = &$this->formState['values'];
    $file = isset($form_values['ingest-file-location']) ? $form_values['ingest-file-location'] : '';
    $form['#attributes']['enctype'] = 'multipart/form-data';
    $form['indicator']['ingest-file-location'] = array(
      '#type' => 'file',
      '#title' => t('Upload Document'),
      '#size' => 48,
      '#description' => t('Full text'),
    );
    $form_generator = FormGenerator::CreateFromModel($this->contentModelPid, $this->contentModelDsid);
    $form[FORM_ROOT] = $form_generator->generate($this->formName); // TODO get from user .
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
    $validators = array();
    $form_values = &$this->formState['values'];
    if (!empty($_FILES['files']['name']['ingest-file-location'])) {
      $file_object = file_save_upload('ingest-file-location', $validators);
      file_move($file_object->filepath, 0, 'FILE_EXISTS_RENAME');
      $form_values['ingest-file-location'] = $file_object->filepath;
    }
    if (file_exists($form_values['ingest-file-location'])) {
      module_load_include('inc', 'fedora_repository', 'ContentModel');
      module_load_include('inc', 'fedora_repository', 'MimeClass');
      $file = $form_values['ingest-file-location'];
      $content_model = ContentModel::loadFromModel($this->contentModelPid, $this->contentModelDsid);
      if ($content_model !== FALSE) {
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
    }
    $this->formState['rebuild'] = FALSE;
    return;
  }

  /**
   *
   * @param array
   *  $form
   */
  public function submitForm(&$form) {
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    $error = TRUE;
    $should_redirect = TRUE;
    $collection_policy = CollectionPolicy::loadFromCollection($this->collectionPid);
    if ($collection_policy !== FALSE) {
      module_load_include('inc', 'fedora_repository', 'ContentModel');
      $relationship = $collection_policy->getRelationship();
      $content_model = ContentModel::loadFromModel($this->contentModelPid, $this->contentModelDsid);
      if ($content_model !== FALSE) {
        module_load_include('inc', 'islandora_form_builder', 'FoxmlForm');
        list($class, $dsid) = $this->loadDocumentClass($content_model);
        $label = "";
        $pid = $collection_policy->getNextPid($this->contentModelDsid);
        $ingest_file_location = isset($this->formState['values']['ingest-file-location']) ? $this->formState['values']['ingest-file-location'] : NULL;
        $document = new $class($form, $this->formState);
        $transform = drupal_get_path('module', 'islandora_form_builder') . '/transforms/mods_to_dc.xsl';
        $foxml = new FoxmlDocument($label, $pid, $dsid, $this->contentModelPid, $this->collectionPid, $relationship, $ingest_file_location, $document, $transform);
        $error = $foxml->ingest();
        $_SESSION['fedora_ingest_files'] = ''; //empty this variable
        $this->formState['storage'] = NULL;
      }
    }
    if ($should_redirect) {
      global $base_url;
      $this->formState['redirect'] = ($error) ? '' : $base_url . "/fedora/repository/{$this->collectionPid}";
    }
  }

  /**
   * Loads the appropriate class file, and returns the class name.
   * 
   * @param ContentModel $content_model
   * @return string
   */
  private function loadDocumentClass($content_model) {
    $form = $content_model->getForm($this->formName);
    $module = $form->getAttribute('module');
    $file = $form->getAttribute('file');
    $class = $form->getAttribute('class');
    $dsid = $form->getAttribute('dsid');
    $path = drupal_get_path('module', $module);
    require_once "$path/$file";
    return array($class, $dsid);
  }

}
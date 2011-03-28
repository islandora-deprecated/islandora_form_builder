<?php

// $Id$

/*
 * @file
 *
 */
module_load_include('inc', 'islandora_form_builder', 'Utilities');
module_load_include('inc', 'islandora_form_builder', 'FormBuilderInterface');
module_load_include('inc', 'fedora_repository', 'api/fedora_item');

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
   * The datatstrean ID for the datastream we will modify.
   *
   * @var string
   */
  protected $dsid;
  /**
   * The name of the form to create.
   * 
   * @var string 
   */
  protected $formName;
  /**
   * The fedora object to edit.
   *
   * @var Fedora_Item
   */
  protected $fedoraItem;

  /**
   * Create the Edit Form Builder.
   *
   * @param array $form_state
   * @param string $pid
   * @param string $dsid
   */
  public function __construct(&$form_state, $pid = NULL, $dsid = NULL, $form_name = NULL) {
    $this->initializeStorage($form_state, $pid, $dsid, $form_name);
    $this->formState = &$form_state;
    $this->storage = &$this->formState['storage'][STORAGE_KEY];
    $this->pid = $this->storage['pid'];
    $this->dsid = $this->storage['dsid'];
    $this->formName = $this->storage['form_name'];
    $this->fedoraItem = new Fedora_Item($this->pid);
  }

  /**
   * Initialize the form's persistant storage.
   *
   * @param array $form_state
   * @param string $pid
   * @param string $dsid
   */
  private function initializeStorage(&$form_state, $pid = NULL, $dsid = NULL, $form_name = NULL) {
    if (!is_array($form_state['storage'][STORAGE_KEY])) {
      $form_state['storage'][STORAGE_KEY] = array();
    }
    $storage = &$form_state['storage'][STORAGE_KEY];
    $storage['pid'] = isset($storage['pid']) ? $storage['pid'] : $pid;
    $storage['dsid'] = isset($storage['dsid']) ? $storage['dsid'] : $dsid;
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
    module_load_include('inc', 'islandora_form_builder', 'FormGenerator');
    module_load_include('inc', 'islandora_form_builder', 'FormPopulator');
    $form_generator = FormGenerator::CreateFromObject($this->pid);
    $form[FORM_ROOT] = $form_generator->generate($this->formName);
    $form_populator = new FormPopulator($this->formState, $this->pid, $this->dsid, $this->formName);
    $form_populator->populate($form[FORM_ROOT]);
    $form['submit'] = array(
      '#type' => 'submit',
      '#submit' => array('islandora_form_builder_ingest_edit_form_submit'),
      '#value' => 'Save Metadata'
    );
    return $form;
  }

  /**
   * Not currently used...
   * 
   * @param array $form
   *   Drupal form to be validated.
   */
  public function validateForm(&$form) {
    
  }

  /**
   * Submit the form. Modify the datastream identified by $this->dsid.
   *
   * @param array $form
   *   Drupal form to be validated.
   */
  public function submitForm(&$form) {
    global $base_url;
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    $content_model = ContentModel::loadFromObject($this->pid);
    list($class) = $this->loadDocumentClass($content_model);
    $document = new $class($form, $this->formState);
    $this->modifyDatastream($document, "{$this->dsid}", "{$this->dsid} Record", 'text/xml');
    $transform = drupal_get_path('module', 'islandora_form_builder') . '/transforms/mods_to_dc.xsl'; // TODO add transform to content modeler ...
    $dublin_core = $this->transformDocumentIntoDublinCore($document, $transform);
    $dublin_core->version = "1.0";
    $dublin_core->encoding = "UTF-8";
    $dublin_core->formatOutput = TRUE;
    $this->modifyDatastream($dublin_core, 'DC', "Dublin Core Record", 'text/xml');
    $this->formState['storage'] = NULL;
    $this->formState['redirect'] = $base_url . '/fedora/repository/' . $this->pid;
  }

  /**
   * Transforms $document in to a dublin core document via xsl transform.
   *
   * @param DOMDocument $document
   * @param string $transform
   *
   * @return DOMDocument
   */
  private function transformDocumentIntoDublinCore($document, $transform) {
    $xsl = new DOMDocument();
    $xsl->load($transform);
    $xslt = new XSLTProcessor();
    $xslt->importStyleSheet($xsl);
    $doc = new DOMDocument();
    $doc->loadXML($document->saveXML());
    return $xslt->transformToDoc($doc->documentElement);
  }

  /**
   * Test if a data stream is managed or not.
   *
   * @param $dsid
   *   Data stream identifier.
   *
   * @return
   *  TRUE if the data stream specified by $disd is managed, FALSE otherwise.
   */
  private function isDatastreamManaged($dsid) {
    $datastream = $this->fedoraItem->get_datastream($dsid);
    return $datastream->controlGroup == 'M';
  }

  /**
   * Overwrites a managed data stream by reference.
   *
   * Only works for managed data streams. Creates a temp file to be used as the
   * reference
   *
   * @param $dom
   *   XML document that will replace the data stream.
   * @param $dsid
   *   Data stream identifier.
   * @param $label
   *   Data stream label.
   * @param $mime_type
   *   Data stream mime_type.
   */
  private function modifyManagedDatastream($dom, $dsid, $label, $mime_type) {
    $temp_file_dir = file_directory_temp();
    $temp_file_path = file_create_filename("modsdatastream.xml", $temp_file_dir);
    if ($dom->save($temp_file_path)) {
      $temp_file_url = file_create_url($temp_file_path);
      if ($this->fedoraItem->modify_datastream_by_reference($temp_file_url, $dsid, $label, $mime_type) !== NULL) {
        drupal_set_message(t('Successfully updated %dsid datastream for object %pid', array('%pid' => $this->pid, '%dsid' => $dsid)));
      }
    }
  }

  /**
   * Overwrites a inlined data stream by value.
   *
   * Only works for inlined data streams.
   *
   * @param $dom
   *   XML document that will replace the data stream.
   * @param $dsid
   *   Data stream identifier.
   * @param $label
   *   Data stream label.
   * @param $mime_type
   *   Data stream mime_type.
   */
  private function modifyInlineDatastream($dom, $dsid, $label, $mime_type) {
    if ($this->fedoraItem->modify_datastream_by_value($dom->saveXML(), $dsid, $label, $mime_type) !== NULL) {
      drupal_set_message(t('Successfully updated %dsid datastream for object %pid', array('%pid' => $this->pid, '%dsid' => $dsid)));
    }
  }

  /**
   * Overwrites a data stream.
   *
   * Overwrites the given datastream by value for inline datastreams and by
   * reference for managed datastreams.
   *
   * @param $dom
   *   XML document that will replace the data stream.
   * @param $dsid
   *   Data stream identifier.
   * @param $label
   *   Data stream label.
   * @param $mime_type
   *   Data stream mime_type.
   */
  private function modifyDatastream($dom, $dsid, $label, $mime_type) {
    if ($this->isDatastreamManaged($dsid)) {
      $this->modifyManagedDatastream($dom, $dsid, $label, $mime_type);
    }
    else {
      $this->modifyInlineDatastream($dom, $dsid, $label, $mime_type);
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
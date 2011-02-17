<?php

// $Id$

/*
 * @file
 *
 * Defines useful constants for this modules as well as helper functions that don't
 * really belong anywhere else.
 */
define('STORAGE_KEY', 'islandora_form_builder');
define('FORM_ROOT', 'form_builder');

/**
 * Sets up the bread crumbs for drupal.
 *
 * @param string $pid
 *   Pid for either a collection or content model.
 */
function islandora_form_builder_set_bread_crumbs($pid) {
  $breadcrumbs = array();
  $object_helper = new ObjectHelper();
  $object_helper->getBreadcrumbs($pid, $breadcrumbs);
  drupal_set_breadcrumb(array_reverse($breadcrumbs));
}

/**
 * Takes a url argument passed to a MENU_CALLBACK, and checks if it is a valid Collection Pid.
 * If it is not valid an attempt will be made to convert it to a valide Collection Pid.
 *
 * @param string $collection_pid
 *   Returns a valid Collection pid from the url argument if possible,
 *   otherwise it returns FALSE.
 */
function islandora_form_builder_validate_url_argument_collection_pid($collection_pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  if (!user_access('ingest new fedora objects')) {
    drupal_set_message(t('You do not have permission to ingest.'), 'error');
    return FALSE;
  }
  if (!validPid($collection_pid)) {
    if (validPid(urldecode($collection_pid))) {
      $collection_pid = urldecode($collection_pid);
    }
    else {
      drupal_set_message(t("This collection PID $collection_pid is not valid"), 'error');
      return FALSE;
    }
  }
  if ($collection_pid == NULL) {
    drupal_set_message(t('You must specify a collection object pid to ingest an object.'), 'error');
    return FALSE;
  }
  return $collection_pid;
}

/**
 * Checks if the users has access to edit data streams, and if the arguments are valid.
 *
 * @global string $user
 *   The current user.
 * @param string $pid
 *   The objects pid.
 * @param string $dsid
 *   The datastream the edit form will be modifing.
 * 
 * @return boolean
 *   TRUE if the user has permission to edit the datastream, and the arguments are valid,
 *   FALSE otherwise.
 */
function islandora_form_builder_check_edit_form_arguments_and_permissions($pid, $dsid, $form_name) {
  global $user;
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  if ($pid == NULL || $dsid == NULL || $form_name == NULL) {
    drupal_set_message(t('You must specify an object pid a XML based dsid and a form name to edit.'), 'error');
    return FALSE;
  }
  else if (!fedora_repository_access(OBJECTHELPER :: $EDIT_FEDORA_METADATA, $pid, $user)) {
    drupal_set_message(t("You do not have access to Fedora objects within the attempted namespace or you do not have permission to edit meta data for this object."), 'error');
    return FALSE;
  }
  return TRUE;
}
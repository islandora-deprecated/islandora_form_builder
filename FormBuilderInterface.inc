<?php

// $Id$

/*
 * @file
 * Provides an interface for users of this module should they choose to create thier own form builders.
 */

/**
 * Provides an interface for users of this module should they choose to create thier own form builders.
 */
interface FormBuilderInterface {
  public function createForm();
  public function validateForm(&$form);
  public function submitForm(&$form);
}
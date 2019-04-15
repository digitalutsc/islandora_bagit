<?php

/**
 * @file
 * Plugin for the Islandora BagIt Drupal module.
 *
 * Exports the FOXML for an Islandora object and registers it with the
 * Bag so it is copied into the 'data' directory.
 *
 * See https://wiki.duraspace.org/display/FEDORA35/REST+API#RESTAPI-export
 * for possible values of 'format' and 'context' parameters in the call to
 * the Tuque API-M's 'export' method.
 */

/**
 * Returns an array of source and destination file paths.
 *
 * @param object $islandora_object
 *   The Islandora object to create a Bag for.
 *
 * @param string $tmp_ds_directory
 *   The temporary directory where the datastream files have been downloaded.
 *
 * @return array|bool
 *   An array of source and destination file paths, or FALSE
 *   if no datastream files are present.
 */
function islandora_bagit_plugin_object_foxml_atomzip_init($islandora_object, $tmp_ds_directory) {
  // We don't call islandora_bagit_retrieve_datastreams() here because this
  // plugin doesn't add any datastreams.
  $files_to_add = array();

  // Use the Tuque API-M to export the FOXML Atom Zip for the object.
  global $user;
  $fedora_url = variable_get('islandora_base_url', 'http://localhost:8080/fedora');
  $connection = islandora_get_tuque_connection($user, $fedora_url);
  $foxml_atomzip = $connection->api->m->export($islandora_object->id,
    array(
      'format' => 'info:fedora/fedora-system:ATOMZip-1.1',
      'context' => 'archive',
      'encoding' => 'UTF-8',
    )
  );

  $foxml_atomzip_path = $tmp_ds_directory . DIRECTORY_SEPARATOR . 'foxml_atomzip.zip';
  file_put_contents($foxml_atomzip_path, $foxml_atomzip);
  $files_to_add[] = array(
    'source' => $foxml_atomzip_path,
    'dest' => 'foxml_atomzip.zip',
  );

  if (count($files_to_add)) {
    return $files_to_add;
  }
  else {
    return FALSE;
  }
}


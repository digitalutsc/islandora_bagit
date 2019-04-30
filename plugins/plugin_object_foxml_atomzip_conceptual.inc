<?php

/**
 * @file
 * Plugin for the Islandora BagIt Drupal module.
 *
 * Exports the FOXMLZip for an Islandora object and registers it with the
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
function islandora_bagit_plugin_object_foxml_atomzip_conceptual_init($islandora_object, $tmp_ds_directory) {
  // We don't call islandora_bagit_retrieve_datastreams() here because this
  // plugin doesn't add any datastreams.
  $files_to_add = array();
  $members = array();

  // Use the Tuque API-M to export the FOXML for the object.
  global $user;
  $fedora_url = variable_get('islandora_base_url', 'http://localhost:8080/fedora');
  $connection = islandora_get_tuque_connection($user, $fedora_url);

  // Add the actual book or compound object
  $obj_pid = $islandora_object->id;
  $files_to_add = get_atomzip($connection, $tmp_ds_directory, $obj_pid, $files_to_add);

  // If it is bookCModel, get objects
  $b_book_model = in_array('islandora:bookCModel', $islandora_object->models);
  if ($b_book_model) {
    $predicate = "<http://islandora.ca/ontology/relsext#isPageOf>";
    $members = get_related_objects($obj_pid, $predicate);
  }

  // If it is compoundCModel, get child objects
  $b_compound_model = in_array('islandora:compoundCModel', $islandora_object->models);
  if ($b_compound_model) {
    $predicate = "<fedora-rels-ext:isConstituentOf>";
    $members = get_related_objects($obj_pid, $predicate);
  }

  // Loop through each related object and get its atom zip and put into files_to_add
  foreach ($members as $member) {
    $member_pid = $member['object']['value'];
    $files_to_add = get_atomzip($connection, $tmp_ds_directory, $member_pid, $files_to_add);
  }

  if (count($files_to_add)) {
    return $files_to_add;
  }
  else {
    return FALSE;
  }
}

/**
 * Exports the fedora object and array with points to the exported atom zip file.
 *
 * @param $connection
 * @param $tmp_ds_directory
 * @param $pid
 * @param $files_to_add
 * @return array
 */
function get_atomzip($connection, $tmp_ds_directory, $pid, $files_to_add) {
  $foxml_atomzip = $connection->api->m->export($pid,
    array(
      'format' => 'info:fedora/fedora-system:ATOMZip-1.1',
      'context' => 'archive',
      'encoding' => 'UTF-8',
    )
  );

  $pid_s = str_replace(array(':', '-'), '_', $pid);

  $file_name = $pid_s . "_foxml_atomzip.zip";

  $foxml_atomzip_path = $tmp_ds_directory . DIRECTORY_SEPARATOR . $file_name;
  file_put_contents($foxml_atomzip_path, $foxml_atomzip);
  $files_to_add[] = array(
    'source' => $foxml_atomzip_path,
    'dest' => $file_name,
  );
  return $files_to_add;
}

/**
 * Queries Triplestore to get related objects
 * For Book, all page objects.
 * For CompoundObject, all child objects.
 *
 * @param $obj_pid
 * @param $predicate
 * @return array
 */
function get_related_objects($obj_pid, $predicate) {
  $tuque = islandora_get_tuque_connection();
  $ri_query = <<<EOQ
SELECT ?object
FROM <#ri>
WHERE {
  ?object $predicate <info:fedora/$obj_pid> 
}
EOQ;
  $members = [];

  try {
    $members = $tuque->repository->ri->sparqlQuery($ri_query, 'unlimited');
  } catch (Exception $e) {
    watchdog("islandora_bagit", "error in sparqlauery");
  }

  return $members;
}

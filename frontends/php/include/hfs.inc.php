<?php

require_once "include/defines.inc.php";


// check that HFS module is loaded
function zbx_hfs_available ()
{
	return function_exists ("zabbix_hfs_host_availability");
}


// Routine returns correct item's status and error.
// db_item array must have 'itemid' and 'sitename' entries.
function zbx_hfs_item_status ($db_item)
{
	$res = array ("status" => $db_item["status"], "error" => $db_item["error"]);

	if (!zbx_hfs_available ())
		return $res;

	$hfs_status = zabbix_hfs_item_status ($db_item["sitename"], $db_item["itemid"]);

	// we should keep in mind that we may have updated status in HFS
	if (is_object ($hfs_status) && $hfs_status->status == ITEM_STATUS_NOTSUPPORTED)
		$res = array ("status" => ITEM_STATUS_NOTSUPPORTED, "error" => $hfs_status->error);

	return $res;
}



// Routine returns correct item's stderr
// db_item array must have 'itemid' and 'sitename' entries.
function zbx_hfs_item_stderr ($db_item)
{
	if (!zbx_hfs_available ())
		return $db_item["stderr"];

	$stderr = zabbix_hfs_item_stderr ($db_item["sitename"], $db_item["itemid"]);

	// we should keep in mind that we may have updated status in HFS
	if (is_object ($stderr))
		return $stderr->stderr;

	return $db_item["stderr"];
}


?>
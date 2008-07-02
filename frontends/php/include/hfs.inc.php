<?php

require_once "include/defines.inc.php";
require_once "include/db.inc.php";


// check that HFS module is loaded
function zbx_hfs_available ()
{
	return function_exists ("zabbix_hfs_host_availability");
}


// Routine returns correct hosts's availability and error.
// db_item array must have 'hostid' and 'sitename' entries.
function zbx_hfs_host_availability ($db_item)
{
	$res = array ("available" => $db_item["available"], "error" => $db_item["error"]);

	if (!zbx_hfs_available ()) 
		return $res;

	$hfs_status = zabbix_hfs_host_availability ($db_item["sitename"], $db_item["hostid"]);

	if (is_object ($hfs_status))
		$res = array ("available" => $hfs_status->available, "error" => $hfs_status->error);

	return $res;
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


function zbx_hfs_get_item_values ($db_item)
{
	$res = array ("lastclock" => $db_item["lastclock"], "prevvalue" => $db_item["prevvalue"], "lastvalue" => $db_item["lastvalue"]);

	if (!zbx_hfs_available ())
		return $res;

	$val = zabbix_hfs_item_values ($db_item["sitename"], $db_item["itemid"], $db_item["value_type"]);

	if (is_array ($val))
		return $val;

	return $res;
}


function zbx_hfs_last ($site, $itemid, $count, $type)
{
	// HFS not loaded, use regular way to retrieve items
	if (!zbx_hfs_available ()) {
		switch($item_type) {
		case ITEM_VALUE_TYPE_FLOAT:	$h_table = "history";		break;
		case ITEM_VALUE_TYPE_UINT64:	$h_table = "history_uint";	break;
		case ITEM_VALUE_TYPE_TEXT:	$h_table = "history_text";	break;
		default:			$h_table = "history_str";
		}	

		$result = DBselect("select h.clock,h.value,i.valuemapid from $h_table h, items i".
				   " where h.itemid=i.itemid and i.itemid=".$_REQUEST["itemid"].
				   " order by clock desc", $count);
		while ($row = DBfetch($result)) {
			$val->value = $row["value"];
			$val->clock = $row["clock"];
			array_push ($arr, $val);
		
		}

		return $arr;
	}

	if ($type == ITEM_VALUE_TYPE_TEXT || $type == ITEM_VALUE_TYPE_STR)
		return zabbix_hfs_last_str ($site, $itemid, $count);
	else
		return zabbix_hfs_last ($site, $itemid, $count);
}


function zbx_hfs_get_trigger_value ($row, $site, $triggerid)
{
	if (!zbx_hfs_available ())
		return $row;

	$obj = zabbix_hfs_trigger_value ($site, $triggerid);

	if (!is_object ($obj))
		return $row;

	$row["value"] = $obj->value;
	$row["lastchange"] = $obj->when;

	return $row;
}

?>
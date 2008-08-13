<?php
	require_once "include/db.inc.php";


// check that HFS module is loaded
function zbx_hfs_available ()
{
	return function_exists ("zabbix_hfs_read_history");
}

function zbx_hfs_sites ()
{
	$sites = DBselect ('select name from sites');
	while ($site = DBfetch ($sites)) {
		$res[] = $site["name"];
	}

	return $res;
}

?>

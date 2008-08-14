<?php
	require_once "include/db.inc.php";


// check that HFS module is loaded
function zbx_hfs_available ()
{
	return function_exists ("zabbix_hfs_read_history");
}

function zbx_hfs_sites ($groupid, $hostid)
{
	if ($groupid == 0 && $hostid == 0)
		$sql = 'select name from sites';
	else
		if ($hostid == 0)
			$sql = "select s.name from hosts h, hosts_groups hg, sites s where hg.hostid = h.hostid and ".
				"hg.groupid = $groupid and s.siteid = h.siteid and h.status != 3 group by name";
		else
			$sql = "select s.name from hosts h, sites s where h.hostid = $hostid and s.siteid = h.siteid and h.status != 3";

        $res = array ();
	$sites = DBselect ($sql);
	while ($site = DBfetch ($sites)) {
		$res[] = $site["name"];
	}

	return $res;
}

?>

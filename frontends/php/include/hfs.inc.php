<?php
	require_once "include/db.inc.php";


// check that HFS module is loaded
function zbx_hfs_available ()
{
	return function_exists ("zabbix_hfs_read_history");
}


function zbx_item_site ($itemid)
{
	$res = DBselect ("select s.name from items i, hosts h, sites s where i.itemid = $itemid and i.hostid = h.hostid and h.siteid = s.siteid");

	if ($row = DBfetch ($res))
		return $row["name"];
	else
		return "";
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


function zbx_hfs_hosts_availability ($groupid)
{
	$hfs_statuses = array ();

	foreach (zbx_hfs_sites ($groupid, 0) as $site) {
		$res = zabbix_hfs_hosts_availability ($site);

		foreach ($res as $id => $val) {
			if (array_key_exists ($id, $hfs_statuses)) {
				if ($hfs_statuses[$id]->last < $val->last)
					$hfs_statuses[$id] = $val;
			}
			else
				$hfs_statuses[$id] = $val;
		}
	}

	return $hfs_statuses;
}


function zbx_hfs_triggers ($groupid, $hostid)
{
	$hfs_triggers = array ();

	foreach (zbx_hfs_sites ($groupid, 0) as $site) {
		$res = zabbix_hfs_triggers_values ($site);

		foreach ($res as $id => $val) {
			if (array_key_exists ($id, $hfs_triggers)) {
				if ($hfs_triggers[$id]->when < $val->when)
					$hfs_triggers[$id] = $val;
			}
			else
				$hfs_triggers[$id] = $val;
		}
	}

	return $hfs_triggers;
}


?>

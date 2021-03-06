<?php
/* 
** ZABBIX
** Copyright (C) 2000-2005 SIA Zabbix
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**/
?>
<?php
	require_once "include/config.inc.php";
	require_once "include/hosts.inc.php";
	require_once "include/forms.inc.php";

	$page["title"] = "S_DISKS_PROFILES";
	$page["file"] = "disksprofiles.php";
	
include_once "include/page_header.php";

?>
<?php
//		VAR			TYPE	OPTIONAL FLAGS	VALIDATION	EXCEPTION
	$fields=array(
		"groupid"=>		array(T_ZBX_INT, O_OPT,	P_SYS,	DB_ID,	NULL),
		"astext"=>		array(T_ZBX_INT, O_OPT,	NULL,	IN('0,1'),	NULL)
	);

	check_fields($fields);

	validate_group(PERM_READ_ONLY, array("allow_all_hosts","always_select_first_host","monitored_hosts","with_items"));
?>
<?php
	$r_form = new CForm();
	$r_form->SetMethod('get');

	$cmbGroup = new CComboBox("groupid",$_REQUEST["groupid"],"submit()");

	$cmbGroup->AddItem(0,S_ALL_SMALL);
	
	$availiable_groups = get_accessible_groups_by_user($USER_DETAILS,PERM_READ_LIST);

	$result=DBselect("select distinct g.groupid,g.name from groups g, hosts_groups hg, hosts h, items i ".
		" where hg.groupid in (".$availiable_groups.") ".
		" and hg.groupid=g.groupid and h.status=".HOST_STATUS_MONITORED.
		" and h.hostid=i.hostid and hg.hostid=h.hostid ".
		" order by g.name");
	while($row=DBfetch($result))
	{
		$cmbGroup->AddItem($row['groupid'], $row['name']);
	}
	$r_form->AddItem(array(S_GROUP.SPACE,$cmbGroup));
	
	show_table_header(S_DISKS_PROFILES_BIG, $r_form);
?>

<?php
	$table = new CTableInfo();
	$table->setHeader(array(S_HOST,S_UPDATE_TIME,S_DISKS_SERIALS));
$table->AddRow(array ("shmuma.yandex.ru", "Now", "asd, asd, asd, asd"));
// 		if($_REQUEST["groupid"] > 0)
// 		{
// 			$sql="select h.hostid,h.host,p.name,p.os,p.serialno,p.tag,p.macaddress".
// 				" from hosts h,hosts_profiles p,hosts_groups hg where h.hostid=p.hostid".
// 				" and h.hostid=hg.hostid and hg.groupid=".$_REQUEST["groupid"].
// 				" and hg.hostid in (".$availiable_groups.") ".
// 				" order by h.host";
// 		}
// 		else
// 		{
// 			$sql="select h.hostid,h.host,p.name,p.os,p.serialno,p.tag,p.macaddress".
// 				" from hosts h,hosts_groups hg,hosts_profiles p where h.hostid=p.hostid and hg.hostid=h.hostid ".
// 				" and hg.groupid in (".$availiable_groups.") ".
// 				" order by h.host";
// 		}

// 		$result=DBselect($sql);
// 		while($row=DBfetch($result))
// 		{
// 			$table->AddRow(array(
// 				new CLink($row["host"],"?hostid=".$row["hostid"].url_param("groupid"),"action"),
// 				$row["name"],
// 				$row["os"],
// 				$row["serialno"],
// 				$row["tag"],
// 				$row["macaddress"]
// 				));
// 		}
		$table->show();
?>
<?php

include_once "include/page_footer.php";

?>

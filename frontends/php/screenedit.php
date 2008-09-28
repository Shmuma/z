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
	require_once "include/screens.inc.php";
	require_once "include/forms.inc.php";

	$page["title"] = "S_CONFIGURATION_OF_SCREENS";
	$page["file"] = "screenedit.php";

include_once "include/page_header.php";
	
?>
<?php

//		VAR			TYPE	OPTIONAL FLAGS	VALIDATION	EXCEPTION
	$fields=array(
		"screenid"=>	array(T_ZBX_INT, O_MAND, P_SYS,	DB_ID,		null),
		
		"screenitemid"=>array(T_ZBX_INT, O_OPT,	 P_SYS,	DB_ID,			'(isset({form})&&({form}=="update"))&&(!isset({x})||!isset({y}))'),
		"resourcetype"=>	array(T_ZBX_INT, O_OPT,  null,  
					BETWEEN(SCREEN_RESOURCE_GRAPH,SCREEN_RESOURCE_EVENTS),	'isset({save})'),
		"resourceid"=>	array(T_ZBX_INT, O_OPT,  null,  DB_ID,			'isset({save})'),
		"width"=>	array(T_ZBX_INT, O_OPT,  null,  BETWEEN(0,65535),	null),
		"height"=>	array(T_ZBX_INT, O_OPT,  null,  BETWEEN(0,65535),	null),
		"colspan"=>	array(T_ZBX_INT, O_OPT,  null,  BETWEEN(0,100),		null),
		"rowspan"=>	array(T_ZBX_INT, O_OPT,  null,  BETWEEN(0,100),		null),
		"elements"=>	array(T_ZBX_INT, O_OPT,  null,  BETWEEN(1,65535),	null),
		"valign"=>	array(T_ZBX_INT, O_OPT,  null,  
					BETWEEN(VALIGN_MIDDLE,VALIGN_BOTTOM),		null),
		"halign"=>	array(T_ZBX_INT, O_OPT,  null,  
					BETWEEN(HALIGN_CENTER,HALIGN_RIGHT),		null),
		"style"=>	array(T_ZBX_INT, O_OPT,  null,  
					BETWEEN(STYLE_HORISONTAL,STYLE_VERTICAL),	'isset({save})'),
		"url"=>		array(T_ZBX_STR, O_OPT,  null,  null,			'isset({save})'),
		"x"=>		array(T_ZBX_INT, O_OPT,  null,  BETWEEN(1,100),		'isset({save})&&(isset({form})&&({form}!="update"))'),
		"y"=>		array(T_ZBX_INT, O_OPT,  null,  BETWEEN(1,100),		'isset({save})&&(isset({form})&&({form}!="update"))'),

		"save"=>		array(T_ZBX_STR, O_OPT, P_SYS|P_ACT,	null,	null),
		"delete"=>		array(T_ZBX_STR, O_OPT, P_SYS|P_ACT,	null,	null),
		"cancel"=>		array(T_ZBX_STR, O_OPT, P_SYS,	null,	null),
		"form"=>		array(T_ZBX_STR, O_OPT, P_SYS,	null,	null),
		"form_refresh"=>	array(T_ZBX_INT, O_OPT,	null,	null,	null),

		"groupid"=>		array(T_ZBX_INT, O_OPT,	P_SYS,	DB_ID,		NULL),
		"hostid"=>		array(T_ZBX_INT, O_OPT,	P_SYS,	DB_ID,		NULL),
		"select"=>		array(T_ZBX_STR, O_OPT, NULL,	NULL,		NULL)
	);

	check_fields($fields);
	$options = array("allow_all_hosts","always_select_first_host","monitored_hosts","with_monitored_items");
?>
<?php
	$_REQUEST["select"] = get_request("select","");

	if (!isset ($_REQUEST["groupid"]))
		$_REQUEST["groupid"] = 0;
	if (!isset ($_REQUEST["hostid"]))
		$_REQUEST["hostid"] = 0;

	$r_form = new CForm();
	$r_form->SetMethod('get');

	$r_form->AddVar("select",$_REQUEST["select"]);
	$r_form->AddVar("screenid",$_REQUEST["screenid"]);
	if (isset ($_REQUEST["screenitemid"]))
		$r_form->AddVar("screenitemid",$_REQUEST["screenitemid"]);

	$cmbGroup = new CComboBox("groupid",$_REQUEST["groupid"],"submit()");
	$cmbHosts = new CComboBox("hostid",$_REQUEST["hostid"],"submit()");

	$availiable_groups= get_accessible_groups_by_user($USER_DETAILS,PERM_READ_LIST, null, null, get_current_nodeid());

	$result=DBselect("select distinct g.groupid,g.name from groups g, hosts_groups hg, hosts h, items i ".
		" where g.groupid in (".$availiable_groups.") ".
		" and hg.groupid=g.groupid and h.status=".HOST_STATUS_MONITORED.
		" and h.hostid=i.hostid and hg.hostid=h.hostid and i.status=".ITEM_STATUS_ACTIVE.
		" order by g.name");
	while($row=DBfetch($result))
	{
		if ($_REQUEST["groupid"] == 0)
			$_REQUEST["groupid"] = $row["groupid"];
		$cmbGroup->AddItem($row['groupid'], $row['name']);
	}
	$r_form->AddItem(array(S_GROUP.SPACE,$cmbGroup));

	if($_REQUEST["groupid"] > 0)
	{
		$cmbHosts->AddItem(0,S_ALL_SMALL);
		$sql="select distinct h.hostid,h.host from hosts h,items i,hosts_groups hg where h.status=".HOST_STATUS_MONITORED.
			" and h.hostid=i.hostid and hg.groupid=".$_REQUEST["groupid"]." and hg.hostid=h.hostid".
			" and i.status=".ITEM_STATUS_ACTIVE.
			" and hg.groupid in (".$availiable_groups.") ".
			" group by h.hostid,h.host order by h.host";
	}
	else
	{
		$sql="select distinct h.hostid,h.host from hosts h,hosts_groups hg,items i where h.status=".HOST_STATUS_MONITORED.
			" and i.status=".ITEM_STATUS_ACTIVE." and h.hostid=i.hostid".
			" and hg.hostid=h.hostid and hg.groupid in (".$availiable_groups.") ".
			" group by h.hostid,h.host order by h.host";
	}
	$result=DBselect($sql);
	while($row=DBfetch($result))
	{
		$cmbHosts->AddItem($row['hostid'], $row["host"]);
	}
	$r_form->AddItem(array(SPACE.S_HOST.SPACE,$cmbHosts));

	show_table_header(S_CONFIGURATION_OF_SCREEN_BIG, $r_form);

	if(isset($_REQUEST["screenid"]))
	{
		if(!screen_accessiable($_REQUEST["screenid"], PERM_READ_WRITE))
			access_deny();

		$screen = get_screen_by_screenid($_REQUEST["screenid"]);

		echo BR;
		if(isset($_REQUEST["save"]))
		{
			if(!isset($_REQUEST["elements"]))	$_REQUEST["elements"]=0;

			if(isset($_REQUEST["screenitemid"]))
			{
				$result=update_screen_item($_REQUEST["screenitemid"],
					$_REQUEST["resourcetype"],$_REQUEST["resourceid"],$_REQUEST["width"],
					$_REQUEST["height"],$_REQUEST["colspan"],$_REQUEST["rowspan"],
					$_REQUEST["elements"],$_REQUEST["valign"],
					$_REQUEST["halign"],$_REQUEST["style"],$_REQUEST["url"]);

				show_messages($result, S_ITEM_UPDATED, S_CANNOT_UPDATE_ITEM);
			}
			else
			{
				$result=add_screen_item(
					$_REQUEST["resourcetype"],$_REQUEST["screenid"],
					$_REQUEST["x"],$_REQUEST["y"],$_REQUEST["resourceid"],
					$_REQUEST["width"],$_REQUEST["height"],$_REQUEST["colspan"],
					$_REQUEST["rowspan"],$_REQUEST["elements"],$_REQUEST["valign"],
					$_REQUEST["halign"],$_REQUEST["style"],$_REQUEST["url"]);

				show_messages($result, S_ITEM_ADDED, S_CANNOT_ADD_ITEM);
			}
			if($result){
				add_audit(AUDIT_ACTION_UPDATE,AUDIT_RESOURCE_SCREEN," Name [".$screen['name']."] cell changed ".
					(isset($_REQUEST["screenitemid"]) ? "[".$_REQUEST["screenitemid"]."]" : 
						"[".$_REQUEST["x"].",".$_REQUEST["y"]."]"));
				unset($_REQUEST["form"]);
			}
		} elseif(isset($_REQUEST["delete"])) {
			$result=delete_screen_item($_REQUEST["screenitemid"]);
			show_messages($result, S_ITEM_DELETED, S_CANNOT_DELETE_ITEM);
			unset($_REQUEST["x"]);
		}

		if($_REQUEST["screenid"] > 0)
		{
			$table = get_screen($_REQUEST["screenid"], 1);
			$table = $table['table'];
			$table->Show();
		}

	}
?>

<?php

include_once "include/page_footer.php";

?>

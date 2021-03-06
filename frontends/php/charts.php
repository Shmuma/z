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
	require_once "include/graphs.inc.php";

	$page["title"] = "S_CUSTOM_GRAPHS";
	$page["file"] = "charts.php";
?>
<?php
	if(isset($_REQUEST["fullscreen"]))
	{
		define('ZBX_PAGE_NO_MENU', 1);
	}

	if(isset($_REQUEST["graphid"]) && $_REQUEST["graphid"] > 0 && !isset($_REQUEST["period"]) && !isset($_REQUEST["stime"]))
	{
		define('ZBX_PAGE_DO_REFRESH', 1);
	}
	
include_once "include/page_header.php";

?>
<?php
//		VAR			TYPE	OPTIONAL FLAGS	VALIDATION	EXCEPTION
	$fields=array(
		"groupid"=>		array(T_ZBX_INT, O_OPT,	 P_SYS,		DB_ID,NULL),
		"hostid"=>		array(T_ZBX_INT, O_OPT,  P_SYS,		DB_ID,NULL),
		"graphid"=>		array(T_ZBX_INT, O_OPT,  P_SYS,		DB_ID,NULL),
		"dec"=>			array(T_ZBX_INT, O_OPT,  P_SYS, 	BETWEEN(0,65535*65535),NULL),
		"inc"=>			array(T_ZBX_INT, O_OPT,  P_SYS, 	BETWEEN(0,65535*65535),NULL),
		"left"=>		array(T_ZBX_INT, O_OPT,  P_SYS, 	BETWEEN(0,65535*65535),NULL),
		"right"=>		array(T_ZBX_INT, O_OPT,  P_SYS, 	BETWEEN(0,65535*65535),NULL),
		"from"=>		array(T_ZBX_INT, O_OPT,  P_SYS, 	BETWEEN(0,65535*65535),NULL),
		"period"=>		array(T_ZBX_INT, O_OPT,  P_SYS, 	BETWEEN(ZBX_MIN_PERIOD,ZBX_MAX_PERIOD),NULL),
		"stime"=>		array(T_ZBX_STR, O_OPT,  P_SYS, 	NULL,NULL),
		"action"=>		array(T_ZBX_STR, O_OPT,  P_SYS, 	IN("'go'"),NULL),
		"reset"=>		array(T_ZBX_STR, O_OPT,  P_SYS, 	IN("'reset'"),NULL),
		"fullscreen"=>		array(T_ZBX_INT, O_OPT,	P_SYS,		IN("1"),		NULL)
	);

	check_fields($fields);
?>
<?php
	if(isset($_REQUEST["graphid"]) && !isset($_REQUEST["hostid"]))
	{
		$_REQUEST["groupid"] = $_REQUEST["hostid"] = 0;
	}

	$_REQUEST["graphid"] = get_request("graphid", get_profile("web.charts.graphid", 0));
	
	$_REQUEST["keep"] = get_request("keep", 1); // possible excessed REQUEST variable !!!

	$_REQUEST["period"] = get_request("period",get_profile("web.graph.period", ZBX_PERIOD_DEFAULT));
	$effectiveperiod = navigation_bar_calc();

	validate_group_with_host(PERM_READ_ONLY,array("allow_all_hosts","monitored_hosts","with_items"));

	if($_REQUEST["graphid"] > 0 && $_REQUEST["hostid"] > 0)
	{
		$result=DBselect("select g.graphid from graphs g, graphs_items gi, items i".
			" where i.hostid=".$_REQUEST["hostid"]." and gi.itemid = i.itemid".
			" and gi.graphid = g.graphid and g.graphid=".$_REQUEST["graphid"]);
		if(!DBfetch($result))
			$_REQUEST["graphid"] = 0;
	}
?>
<?php
	if($_REQUEST["graphid"] > 0 && $_REQUEST["period"] >= ZBX_MIN_PERIOD)
	{
		update_profile("web.graph.period",$_REQUEST["period"]);
	}

	update_profile("web.charts.graphid",$_REQUEST["graphid"]);
?>
<?php
	$h1 = array(S_GRAPHS_BIG.SPACE."/".SPACE);
	
	$availiable_groups_arr = get_accessible_groups_by_user($USER_DETAILS,PERM_READ_ONLY, null, PERM_RES_IDS_ARRAY, get_current_nodeid());
        $availiable_groups = implode(',', $availiable_groups_arr);
	$denyed_groups = get_accessible_groups_by_user($USER_DETAILS,PERM_READ_ONLY, PERM_MODE_LT);

	if($_REQUEST['graphid'] > 0 && DBfetch(DBselect('select distinct graphid from graphs where graphid='.$_REQUEST['graphid'])))
	{
		if(! ($row = DBfetch(DBselect(" select distinct h.host, g.name, h.hostid, hg.groupid from hosts h, hosts_groups hg, items i, graphs_items gi, graphs g ".
					" where h.status=".HOST_STATUS_MONITORED.
					" and h.hostid=i.hostid and g.graphid=".$_REQUEST["graphid"].
					" and hg.hostid=h.hostid and i.itemid=gi.itemid and gi.graphid=g.graphid".
					      //					" and hg.groupid not in (".$denyed_groups.") ".
					      //					' and '.DBin_node('g.graphid').
					" order by h.host, g.name"
				))))
		{
			update_profile("web.charts.graphid",0);
			access_deny();
		}
		array_push($h1, new CLink($row["name"], "?graphid=".$_REQUEST["graphid"].(isset($_REQUEST["fullscreen"]) ? "&fullscreen=1" : "")));
		$_REQUEST["groupid"] = $row["groupid"];
		$_REQUEST["hostid"] = $row["hostid"];
	}
	else
	{
		$_REQUEST['graphid'] = 0;
		array_push($h1, S_SELECT_GRAPH_TO_DISPLAY);
	}

	$r_form = new CForm();
	$r_form->SetMethod('get');
	if(isset($_REQUEST['fullscreen']))
		$r_form->AddVar('fullscreen', 1);

	$cmbGroup = new CComboBox("groupid",$_REQUEST["groupid"],"submit()");
	$cmbHosts = new CComboBox("hostid",$_REQUEST["hostid"],"submit()");
	$cmbGraph = new CComboBox("graphid",$_REQUEST["graphid"],"submit()");

	$result=DBselect("select distinct g.groupid,g.name from groups g, hosts_groups hg, hosts h, items i, graphs_items gi ".
		" where g.groupid in (".$availiable_groups.") ".
		" and hg.groupid=g.groupid and h.status=".HOST_STATUS_MONITORED.
		" and h.hostid=i.hostid and hg.hostid=h.hostid and i.itemid=gi.itemid ".
		" order by g.name");
        $groups = array ();
	while($row=DBfetch($result))
	{
		$cmbGroup->AddItem($row['groupid'], $row["name"]);
		$groups[] = $row['groupid'];
	}
	$r_form->AddItem(array(S_GROUP.SPACE,$cmbGroup));
	if (!isset ($_REQUEST["graphid"]) || $_REQUEST["graphid"] == 0)
		if (!isset($_REQUEST["groupid"]) || !in_array($_REQUEST["groupid"], $groups))
			$_REQUEST["groupid"] = $groups[0];

	$cmbHosts->AddItem(0,S_ALL_SMALL, $_REQUEST["hostid"] == 0 ? "yes" : "no");
	if($_REQUEST["groupid"] > 0)
	{
		$sql = " select distinct h.hostid,h.host from hosts h,items i,hosts_groups hg, graphs_items gi ".
			" where h.status=".HOST_STATUS_MONITORED.
			" and h.hostid=i.hostid and hg.groupid=".$_REQUEST["groupid"]." and hg.hostid=h.hostid ".
			" and hg.groupid not in (".$denyed_groups.") and i.itemid=gi.itemid".
			" order by h.host";
	}
	else
	{
		$sql = "select distinct h.hostid,h.host from hosts h, hosts_groups hg, items i, graphs_items gi where h.status=".HOST_STATUS_MONITORED.
			" and i.status=".ITEM_STATUS_ACTIVE." and h.hostid=i.hostid and hg.hostid=h.hostid ".
			" and hg.groupid not in (".$denyed_groups.") and i.itemid=gi.itemid".
			" order by h.host";
	}

	$result=DBselect($sql);
        $hosts = array ();
	while($row=DBfetch($result))
	{
		$cmbHosts->AddItem($row['hostid'], $row['host'], $_REQUEST["hostid"] == $row['hostid'] ? "yes" : "no");
		$hosts[] = $row['hostid'];
	}
	if (!isset ($_REQUEST["graphid"]) || $_REQUEST["graphid"] == 0)
	        if (isset($_REQUEST["hostid"]) && !in_array($_REQUEST["hostid"], $hosts))
			$_REQUEST["hostid"] = $hosts[0];

	$r_form->AddItem(array(SPACE.S_HOST.SPACE,$cmbHosts));

	$cmbGraph->AddItem(0,S_ALL_SMALL);
	$need_hostname = 0;

	if($_REQUEST["hostid"] > 0)
	{
		$sql = "select distinct g.graphid,g.name from graphs g,graphs_items gi,items i, hosts_groups hg ".
			" where i.itemid=gi.itemid and g.graphid=gi.graphid and i.hostid=".$_REQUEST["hostid"].
			' and '.DBin_node('g.graphid').
			" and i.hostid=hg.hostid and hg.groupid not in (".$denyed_groups.") ".
			" order by g.name";
	}
	elseif ($_REQUEST["groupid"] > 0)
	{
		$sql = "select distinct g.graphid,g.name,h.host as hostname from graphs g,graphs_items gi,items i,hosts_groups hg,hosts h".
			" where i.itemid=gi.itemid and g.graphid=gi.graphid and i.hostid=hg.hostid and hg.groupid=".$_REQUEST["groupid"].
			" and i.hostid=h.hostid and h.status=".HOST_STATUS_MONITORED.
			' and '.DBin_node('g.graphid').
			" and hg.groupid not in (".$denyed_groups.") ".
			" order by g.name";
		$need_hostname = 1;
	}
	else
	{
		$sql = "select distinct g.graphid,g.name,h.host as hostname from graphs g,graphs_items gi,items i,hosts h,hosts_groups hg".
			" where i.itemid=gi.itemid and g.graphid=gi.graphid ".
			" and i.hostid=h.hostid and h.status=".HOST_STATUS_MONITORED.
			' and '.DBin_node('g.graphid').
			" and hg.hostid=h.hostid and hg.groupid not in (".$denyed_groups.") ".
			" order by g.name";
		$need_hostname = 1;
	}

	$result = DBselect($sql);
	while($row=DBfetch($result))
	{
		if ($need_hostname)
			$hostname = $row['hostname'].': '.$row['name'];
		else
			$hostname = $row['name'];
		$cmbGraph->AddItem($row['graphid'], $hostname);
	}
	
	$r_form->AddItem(array(SPACE.S_GRAPH.SPACE,$cmbGraph));	

	show_table_header($h1, $r_form);
?>
<?php

function show_graph ($table, $graphid, $effectiveperiod)
{
	$row = 	"\n<script language=\"JavaScript\">\n".
		"if(window.innerWidth) width=window.innerWidth; else width=document.body.clientWidth;\n".
		"document.write(\"<IMG SRC='chart2.php?graphid=$graphid".url_param("stime").url_param("from").
		"&period=".$effectiveperiod."&width=\"+(width-108)+\"'>\")\n".
		"</script><br/>";
	$cols = array (New CCol("&nbsp;"), New CCol($row), New CCol("&nbsp;"));
	$table->AddRow ($cols);

	$g = get_graph_by_graphid ($graphid);
	if (in_array("description", $g) && trim ($g['description']) != "")
	{
		$cols = array (New CCol(""), New CCol("<div class=chart_description>" . description_html($g['description']) . "</div>",
						      "description"),
			       New CCol(""));
		$table->AddRow($cols);
	}
}

	$table = new CTableInfo('...','chart');

	$graphid = $_REQUEST["graphid"];
	$count = 0;

	if ($graphid == 0) {
		$result = DBselect($sql);
		while(($row = DBfetch($result)) && ($count < 100))
		{
			show_graph ($table, $row['graphid'], $effectiveperiod);
			$count++;
		}
	}
	else
		show_graph ($table, $graphid, $effectiveperiod);

	if ($count < 100) {
		$table->Show();
		navigation_bar('charts.php',array('groupid','hostid','graphid'));
	}
	else  {
		$table = new CTableInfo(S_TOO_MANY_OBJECTS, 'chart');
		$table->Show ();
	}
?>
<?php

include_once "include/page_footer.php";

?>

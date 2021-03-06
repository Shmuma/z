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
	require_once "include/acknow.inc.php";
	require_once "include/triggers.inc.php";
	require_once "include/hfs.inc.php";

	$page["file"] = "tr_status.php";
	$page["title"] = "S_STATUS_OF_TRIGGERS";

?>
<?php
	$tr_hash = calc_trigger_hash();

	$triggers_hash = get_cookie('zbx_triggers_hash', '0,0');

	$new=explode(",",$tr_hash);
	$old=explode(",",$triggers_hash);

	zbx_set_post_cookie("zbx_triggers_hash",$tr_hash,time()+1800);

	$triggers_hash = get_cookie('zbx_triggers_hash', '0,0');
	
	$new=explode(",",$tr_hash);
	$old=explode(",",$triggers_hash);

	if( $old[1] != $new[1] )
	{
		if( $new[0] < $old[0] )	// Number of trigger decreased
			$status = "off";
		else			// Number of trigger increased
			$status = "on";

		$files_apdx = array(
			5 => 'disaster',
			4 => 'high',
			3 => 'average',
			2 => 'warning',
			1 => 'information',
			0 => 'not_classified');

		$prior_dif = $new[0]-$old[0];

		krsort($files_apdx);
		foreach($files_apdx as $priority => $apdx)
		{
			if(round($prior_dif / pow(100, $priority)) != 0)
			{
				$audio = 'audio/trigger_'.$status.'_'.$apdx.'.wav';
				break;
			}
		}

		if(!isset($audio) || !file_exists($audio))
			$audio = 'audio/trigger_'.$status.'.wav';
	}

?>
<?php
	define('ZBX_PAGE_DO_REFRESH', 1);

	if(isset($_REQUEST["fullscreen"]))
		define('ZBX_PAGE_NO_MENU', 1);
	
include_once "include/page_header.php";
echo '<script type="text/javascript" src="js/blink.js"></script>';
	
?>
<?php
//		VAR			TYPE	OPTIONAL FLAGS	VALIDATION	EXCEPTION
	$fields=array(
		"groupid"=>	array(T_ZBX_INT, O_OPT, P_SYS,	DB_ID, null),
		"hostid"=>	array(T_ZBX_INT, O_OPT, P_SYS,	DB_ID, null),
		"sort"=>	array(T_ZBX_STR, O_OPT,  null,	IN('"host","priority","description","lastchange"'), null),
		"noactions"=>	array(T_ZBX_STR, O_OPT,  null,	IN('"true","false"'), null),
		"compact"=>	array(T_ZBX_STR, O_OPT,  null,	IN('"true","false"'), null),
		"onlytrue"=>	array(T_ZBX_STR, O_OPT,  null,	IN('"true","recent","false"'), null),
		"show_unknown"=>	array(T_ZBX_INT, O_OPT,	P_SYS,	IN(array(0,1)),	null),
		"select"=>	array(T_ZBX_STR, O_OPT,  null,	IN('"true","false"'), null),
		"txt_select"=>	array(T_ZBX_STR, O_OPT,  null,	null, null),
		"fullscreen"=>	array(T_ZBX_STR, O_OPT,  null,	null, null),
		"btnSelect"=>	array(T_ZBX_STR, O_OPT,  null,  null, null)
	);

	check_fields($fields);

	$_REQUEST["onlytrue"]		=	get_request("onlytrue", get_profile("web.tr_status.onlytrue", 'recent'));
	$_REQUEST["noactions"]		=	get_request("noactions", get_profile("web.tr_status.noactions", 'true'));
	$_REQUEST["compact"]		=	get_request("compact", get_profile("web.tr_status.compact", 'true'));
	$_REQUEST['show_unknown']	=	get_request('show_unknown',get_profile('web.tr_status.show_unknown',0));

	$options = array("allow_all_hosts","monitored_hosts","with_monitored_items");
	if(!$ZBX_WITH_SUBNODES)	array_push($options,"only_current_node");
	
	validate_group_with_host(PERM_READ_ONLY,$options,"web.tr_status.groupid","web.tr_status.hostid");

	update_profile("web.tr_status.onlytrue",$_REQUEST["onlytrue"]);
	update_profile("web.tr_status.noactions",$_REQUEST["noactions"]);
	update_profile("web.tr_status.compact",$_REQUEST["compact"]);
	update_profile('web.tr_status.show_unknown',$_REQUEST['show_unknown']);
	
?>
<?php
	$sort		 = get_request('sort',		'priority');
	$noactions	 = get_request('noactions',	'true');
	$compact	 = get_request('compact',	'true');
	$onlytrue	 = get_request('onlytrue',	'recent');
	$show_unknown= get_request('show_unknown',0);
	$select		 = get_request('select',		'false');
	$txt_select	 = get_request('txt_select',	"");
	if($select == 'false') $txt_select = '';

?>
<?php
	$r_form = new CForm();
	$r_form->SetMethod('get');

	$cmbGroup = new CComboBox("groupid",$_REQUEST["groupid"],"submit()");
	$cmbHosts = new CComboBox("hostid",$_REQUEST["hostid"],"submit()");

	$cmbGroup->AddItem(0,S_ALL_SMALL);
	
	$availiable_groups = get_accessible_groups_by_user($USER_DETAILS,PERM_READ_LIST, null, null, get_current_nodeid());

	$result=DBselect("select distinct g.groupid,g.name from groups g, hosts_groups hg, hosts h, items i, functions f, triggers t ".
		" where hg.groupid in (".$availiable_groups.") ".
		" AND hg.groupid=g.groupid AND h.status=".HOST_STATUS_MONITORED.
		" AND h.hostid=i.hostid AND hg.hostid=h.hostid AND i.status=".ITEM_STATUS_ACTIVE.
		" AND i.itemid=f.itemid AND t.triggerid=f.triggerid AND t.status=".TRIGGER_STATUS_ENABLED.
		" order by g.name");
	while($row=DBfetch($result))
	{
		$cmbGroup->AddItem($row['groupid'], $row['name']);
		unset($row);
	}
	$r_form->AddItem(array(S_GROUP.SPACE,$cmbGroup));

	$cmbHosts->AddItem(0,S_ALL_SMALL);
	if($_REQUEST["groupid"] > 0){
		$sql='SELECT distinct h.hostid,h.host '.
			' FROM hosts h,items i,hosts_groups hg, functions f, triggers t '.
			' WHERE h.status='.HOST_STATUS_MONITORED.
				' AND h.hostid=i.hostid '.
				' AND hg.groupid='.$_REQUEST["groupid"].
				' AND hg.hostid=h.hostid'.
				' AND i.status='.ITEM_STATUS_ACTIVE.
				' AND i.itemid=f.itemid '.
				' AND t.triggerid=f.triggerid '.
				' AND t.status='.TRIGGER_STATUS_ENABLED.
				' AND hg.groupid in ('.$availiable_groups.') '.
			' GROUP BY h.hostid,h.host '.
			' ORDER BY h.host';
	}
	else{
		$sql='SELECT h.hostid,h.host '.
			' FROM hosts h,hosts_groups hg,items i, functions f, triggers t '.
			' WHERE h.status='.HOST_STATUS_MONITORED.
				' AND i.status='.ITEM_STATUS_ACTIVE.
				' AND h.hostid=i.hostid'.
				' AND i.itemid=f.itemid '.
				' AND t.triggerid=f.triggerid '.
				' AND t.status='.TRIGGER_STATUS_ENABLED.
				' AND h.hostid=hg.hostid and hg.groupid in ('.$availiable_groups.') '.
			' GROUP BY h.hostid,h.host '.
			' ORDER BY h.host';
	}
	
	$result=DBselect($sql);
	$flag = false;
	while($row=DBfetch($result))
	{
		$flag |= $_REQUEST['hostid'] == $row['hostid'];
		$cmbHosts->AddItem($row['hostid'], $row['host']);
	}
	if(!$flag) $_REQUEST['hostid'] = 0;

	$r_form->AddItem(array(SPACE.S_HOST.SPACE,$cmbHosts));
	$r_form->AddVar('compact',$compact);
	$r_form->AddVar('onlytrue',$onlytrue);
	$r_form->AddVar('show_unknown',$show_unknown);
	$r_form->AddVar('noactions',$noactions);
	$r_form->AddVar('select',$select);
	$r_form->AddVar('txt_select',$txt_select);
	$r_form->AddVar('sort',$sort);
	if(isset($_REQUEST['fullscreen'])) $r_form->AddVar('fullscreen',1);

	show_table_header(
		new CLink(SPACE.S_STATUS_OF_TRIGGERS_BIG.SPACE.date('[H:i:s]',time()),"tr_status.php?onlytrue=$onlytrue&noactions=$noactions".
			"&compact=$compact&sort=$sort".(!isset($_REQUEST["fullscreen"]) ? '&fullscreen=1' : '')),
		$r_form);
	
	if(!isset($_REQUEST["fullscreen"]))
	{
		$left_col = array();
		if ($onlytrue == 'true') {
			$msg = S_SHOW_TRUE_AND_CHANGED;
			$url = 'recent';
		}
		elseif ($onlytrue == 'recent') {
			$msg = S_SHOW_ALL_TRIGGERS;
			$url = 'false';
		}
		else {
			$msg = S_SHOW_ONLY_TRUE;
			$url = 'true';
		}
		array_push($left_col, '[', new CLink($msg,
			"tr_status.php?onlytrue=$url".
			"&noactions=$noactions&compact=$compact&select=$select&txt_select=$txt_select&sort=$sort"
			), ']'.SPACE);
			
		array_push($left_col, '[', new CLink($show_unknown!=1?S_SHOW_UNKNOWN:S_HIDE_UNKNOWN,
			"tr_status.php?show_unknown=".($show_unknown!=1?'1':'0').
			"&onlytrue=$onlytrue&noactions=$noactions&compact=$compact&select=$select&txt_select=$txt_select&sort=$sort"
			), ']'.SPACE);
		
		array_push($left_col, '[', new CLink($noactions != 'true' ? S_HIDE_ACTIONS : S_SHOW_ACTIONS,
			"tr_status.php?noactions=".($noactions != 'true' ? 'true' : 'false').
			"&onlytrue=$onlytrue&show_unknown=$show_unknown&compact=$compact&select=$select&txt_select=$txt_select&sort=$sort"
			), ']'.SPACE);

		array_push($left_col, '[', new CLink($compact != 'true' ? S_HIDE_DETAILS: S_SHOW_DETAILS,
			"tr_status.php?compact=".($compact != 'true' ? 'true' : 'false').
			"&onlytrue=$onlytrue&show_unknown=$show_unknown&noactions=$noactions&select=$select&txt_select=$txt_select&sort=$sort"
			), ']'.SPACE);
		
		array_push($left_col, '[', new CLink($select != 'true' ? S_SELECT : S_HIDE_SELECT,
			"tr_status.php?select=".($select != 'true' ? 'true' : 'false').
			"&onlytrue=$onlytrue&show_unknown=$show_unknown&noactions=$noactions&compact=$compact&txt_select=$txt_select&sort=$sort"
			), ']');
			
		if($select=='true')
		{
			$form = new CForm();
			$form->SetMethod('get');
			
			$form->AddItem(new CTextBox("txt_select",$txt_select,15));
			$form->AddItem(new CButton("btnSelect", "Select"));
			$form->AddItem(new CButton("btnSelect", "Inverse select"));
			$form->AddVar("compact",$compact);
			$form->AddVar("onlytrue",$onlytrue);
			$form->AddVar("noactions",$noactions);
			$form->AddVar("select",$select);
			array_push($left_col,BR,$form);
		}
		show_table_header($left_col);
	}

  	if(isset($_REQUEST["fullscreen"]))
	{
		$triggerInfo = new CTriggersInfo();
		$triggerInfo->HideHeader();
		$triggerInfo->Show();
	}

	if(isset($_REQUEST["fullscreen"]))
	{
		$fullscreen="&fullscreen=1";
	}
	else
	{
		$fullscreen="";
	}
	
	$table  = new CTableInfo();
	$header=array();

	$headers_array = array(
		is_show_subnodes() ? array('simple_label'=>S_NODE) : null,
		$_REQUEST['hostid'] > 0 ? null : 
		array('select_label'=>S_HOST_BIG	, 'simple_label'=>S_HOST,		'sort'=>'host'),
		array('select_label'=>S_NAME_BIG	, 'simple_label'=>S_NAME,		'sort'=>'description'),
		array('simple_label'=>S_STATUS),
		array('select_label'=>S_SEVERITY_BIG	, 'simple_label'=>S_SEVERITY,		'sort'=>'priority'),
		array('select_label'=>S_LAST_CHANGE_BIG	, 'simple_label'=>S_LAST_CHANGE,	'sort'=>'lastchange'),
		array('simple_label'=>($noactions!='true') ? S_ACTIONS : NULL),
		array('simple_label'=>S_ACKNOWLEDGED),
		array('simple_label'=>S_COMMENTS)
		);

	$select_vars = (isset($sort) && $sort=="description") ? "&select=$select&txt_select=$txt_select" : "";
	foreach($headers_array as $el)
	{
		if(isset($el['sort']) && $sort == $el['sort'])
		{
			$descr = $el['select_label'];
		}
		else if(isset($el['sort']))
		{
			$descr = new CLink($el['simple_label'],"tr_status.php?sort=".$el['sort'].
				"&onlytrue=$onlytrue&noactions=$noactions&compact=$compact$select_vars$fullscreen");
		}
		else
		{
			$descr = $el['simple_label'];
		}
		array_push($header,$descr);
		unset($el);
	}
  
	$table->SetHeader($header);
	unset($header);

	switch ($sort)
	{
		case "host":		$sort="order by h.host";	if($_REQUEST["hostid"] <= 0)	break; /* else "description" */
		case "description":	$sort="order by t.description";				break;
		case "priority":	$sort="order by t.priority desc, t.description";	break;
		case "lastchange":	$sort="order by t.lastchange desc, t.priority";		break;
		default:			$sort="order by t.priority desc, t.description";
	}

	$cond  = ($_REQUEST['hostid'] > 0)?' AND h.hostid='.$_REQUEST['hostid'].' ':'';
	$cond .= ($_REQUEST['groupid'] > 0)?' AND hg.groupid='.$_REQUEST['groupid'].' ':'';

	if (!zbx_hfs_available()) {
		if ($onlytrue == 'true')
			$cond .= ' AND ((t.value=1)';
		elseif ($onlytrue == 'recent')
			$cond .= ' AND ((t.value=1) OR (('.time().' - lastchange)<'.TRIGGER_BLINK_PERIOD.'))';
	
		$cond.=($show_unknown == 0)?' AND t.value<>2 ':'';
	}

	if (zbx_hfs_available ())
		$hfs_triggers = zbx_hfs_triggers ($_REQUEST['groupid'], $_REQUEST['hostid']);
	else
		$hfs_triggers = 0;

	$result = DBselect('SELECT DISTINCT t.triggerid,t.status,t.description, '.
							' t.expression,t.priority,t.lastchange,t.comments,t.url,t.value,h.host,s.name as siteid '.
					' FROM triggers t,hosts h,hosts_groups hg,items i,functions f,sites s '.
					' WHERE f.itemid=i.itemid AND h.hostid=i.hostid '.
						' AND t.triggerid=f.triggerid AND t.status='.TRIGGER_STATUS_ENABLED.
						' AND i.status='.ITEM_STATUS_ACTIVE.' AND '.DBin_node('t.triggerid').
						' AND h.hostid=hg.hostid and hg.groupid not in ('.get_accessible_groups_by_user($USER_DETAILS,PERM_READ_ONLY, PERM_MODE_LT).') '. 
						' AND h.siteid = s.siteid '.
						' AND h.status='.HOST_STATUS_MONITORED.' '.$cond.' '.$sort);

	while($row=DBfetch($result))
	{
// Check for dependencies

		if(trigger_dependent($row["triggerid"]))	continue;

		$elements=array();

		$description = expand_trigger_description($row["triggerid"]);

		if(isset($_REQUEST["btnSelect"]) && '' != $txt_select && ((stristr($description, $txt_select)) == ($_REQUEST["btnSelect"]=="Inverse select"))) continue;

		if (is_array ($hfs_triggers))
			if (array_key_exists ($row["triggerid"], $hfs_triggers)) {
				$row["value"] = $hfs_triggers[$row["triggerid"]]->value;
				$row["lastchange"] = $hfs_triggers[$row["triggerid"]]->when;
			}

		// when trigger values got from HFS, we must filter them manually
		if ($show_unknown == 0 && $row["value"] == 2)
			continue;
		if ($onlytrue == 'true') {
			if ($row["value"] != 1)
				continue;
		}
		elseif ($onlytrue == 'recent')
			if (($row["value"] == 0) && ((time ()-$row["lastchange"]) > TRIGGER_BLINK_PERIOD))
				continue;

		if($row["url"] != "")
		{
			$description = new CLink($description, $row["url"]);
		}

		if($compact != 'true')
		{
			$description = array(
				$description, BR, 
				'<font color="#000000" size="-2">', 
				explode_exp($row["expression"],1), 
				'</font>');
		}

		if((time(NULL)-$row["lastchange"])<TRIGGER_BLINK_PERIOD)
			$blink = array(1=>'<a name="blink">',	2=>'</a>');
		else
			$blink = array(1=>'', 		2=>'');
		
		$value = new CSpan($blink[1].trigger_value2str($row["value"]).$blink[2], get_trigger_value_style($row["value"]));

		if($noactions=='true')
		{
			$actions=NULL;
		}
		else
		{
			$actions=array(
				new CLink(S_CHANGE,'triggers.php?form=update&triggerid='.$row["triggerid"].url_param('hostid'),"action")
				);
		}

		$ack = "-";
		if($row["value"] == 1)
		{
			if($event = get_last_event_by_triggerid($row["triggerid"]))
			{
				if($event["acknowledged"] == 1)
				{
					$acks_cnt = DBfetch(DBselect("select count(*) as cnt from acknowledges where eventid=".$event["eventid"]));
					$ack=array(
						new CSpan(S_YES,"off"),
						SPACE."(".$acks_cnt['cnt'].SPACE,
						new CLink(S_SHOW,
							"acknow.php?eventid=".$event["eventid"],"action"),
						")"
						);
				}
				else
				{
					$ack=array(
						new CSpan(S_NO,"on"),
						SPACE."(",
						new CLink(S_ACK,
							"acknow.php?eventid=".$event["eventid"],"action"),
						")"
						);
				}
			}
		}

		$table->AddRow(array(
				get_node_name_by_elid($row['triggerid']),
				$_REQUEST['hostid'] > 0 ? null : $row['host'],
				$description,
				$value,
				new CCol(
					get_severity_description($row["priority"]),
					get_severity_style($row["priority"])),
				new CLink(zbx_date2str(S_DATE_FORMAT_YMDHMS,$row["lastchange"]),"tr_events.php?triggerid=".$row["triggerid"],"action"),
				$actions,
				new CCol($ack,"center"),
				new CLink(($row["comments"] == "") ? S_ADD : S_SHOW,"tr_comments.php?triggerid=".$row["triggerid"],"action")
				));
		unset($row,$description, $actions);
	}
	zbx_add_post_js('blink.init();');
	$table->Show(false);

	show_table_header(S_TOTAL.": ".$table->GetNumRows());
?>
<?php

include_once "include/page_footer.php";

?>

<?php
/*
** ZABBIX
** Copyright (C) 2000-2007 SIA Zabbix
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
	function	event_source2str($sourceid)
	{
		switch($sourceid)
		{
			case EVENT_SOURCE_TRIGGERS:	return S_TRIGGERS;
			case EVENT_SOURCE_DISCOVERY:	return S_DISCOVERY;
			default:			return S_UNKNOWN;
		}
	}

	function events_sort ($a, $b)
	{
		return $b["clock"] - $a["clock"];
	}

	function	get_history_of_triggers_events($start,$num, $groupid=0, $hostid=0)
	{
		global $USER_DETAILS;

		$show_unknown = get_profile('web.events.show_unknown',0);
		
		$sql_from = $sql_cond = "";

	        $availiable_groups= get_accessible_groups_by_user($USER_DETAILS,PERM_READ_LIST, null, null, get_current_nodeid());
// 	        $availiable_hosts = get_accessible_hosts_by_user($USER_DETAILS,PERM_READ_LIST, null, null, get_current_nodeid());
		
		if($hostid > 0)
		{
			$sql_cond = " and h.hostid=".$hostid;
		}
		elseif($groupid > 0)
		{
			$sql_from = ", hosts_groups hg ";
			$sql_cond = " and h.hostid=hg.hostid and hg.groupid=".$groupid;
		}
// 		else
// 		{
// 			$sql_from = ", hosts_groups hg ";
// 			$sql_cond = " and h.hostid in (".$availiable_hosts.") ";
// 		}
	
//---
		$trigger_list = '';
		$sql = 'SELECT DISTINCT t.triggerid,t.priority,t.description,t.expression,h.host '.
			' FROM triggers t, functions f, items i, hosts h '.$sql_from.
			' WHERE '.DBin_node('t.triggerid').
				' AND t.triggerid=f.triggerid and f.itemid=i.itemid '.
				' AND i.hostid=h.hostid '.$sql_cond.
				' AND h.status='.HOST_STATUS_MONITORED;
							
		$rez = DBselect($sql);
		while($rowz = DBfetch($rez)){
			$triggers[$rowz['triggerid']] = $rowz;
			$trigger_list.=$rowz['triggerid'].',';
		}

		if(!empty($triggers)){
			if (zbx_hfs_available ()) {
				// obtain list of hosts to fetch events from
				$sql = "select h.hostid, s.name as siteid from hosts h, sites s ".$sql_from.
					" where h.siteid = s.siteid ".$sql_cond." and h.status=".HOST_STATUS_MONITORED;
				$res = DBselect ($sql);
				$hosts = array ();
				while ($row = DBfetch ($res)) {
					$hosts[$row['hostid']] = new stdClass();
					$hosts[$row['hostid']]->site = $row['siteid'];
					$hosts[$row['hostid']]->begin = 0;
					$hosts[$row['hostid']]->stop = 0;
				}

				print "<pre>\n";
				// collect events
				$done = 0;
				$res_events = array ();
				while (!$done && $num) {
					$done = 1;
					$events = array ();
					foreach ($hosts as $hostid => $obj) {
 						if ($obj->stop)
							continue;
						$done = 0;
						$hfs_events = zabbix_hfs_host_events ($obj->site, $hostid, $obj->begin, 100);
						rsort ($hfs_events);
						
						if (is_array ($hfs_events) && count ($hfs_events) > 0) {
							$events = array_merge ($events, $hfs_events);
							$hosts[$hostid]->begin += count ($hfs_events);
							if (count ($hfs_events) < 100)
								$hosts[$hostid]->stop = 1;
						}
						else
							$hosts[$hostid]->stop = 1;
					}

					if ($done)
						break;

					// sort resulting array by timestamp
					usort ($events, "events_sort");
					$len = count ($events);

					if ($len == 0)
						break;
					if ($start > 0)  {
						if ($len <= $start) {
							$start -= $len;
							$events = array ();
						} else
							array_splice ($events, $start);
						$len = count ($events);
					}

					if ($start == 0) {
						print "$len asd $num, ";
						if ($len > $num) {
							array_splice ($events, 0, $num);
							$len = count ($events);
						}
						$res_events = array_merge ($res_events, $events);
						$num -= $len;
					}
				}
				print "</pre>";
			} else {
				$trigger_list = '('.trim($trigger_list,',').')';
				$sql_cond=($show_unknown == 0)?(' AND e.value<>'.TRIGGER_VALUE_UNKNOWN.' '):('');

				$sql = 'SELECT e.eventid, e.objectid as triggerid,e.clock,e.value '.
					' FROM events e '.
					' WHERE '.zbx_sql_mod('e.object',1000).'='.EVENT_OBJECT_TRIGGER.
					  ' AND e.objectid IN '.$trigger_list.
					  $sql_cond.
					' ORDER BY e.eventid DESC';

				$result = DBselect($sql,($start+$num));
				$res_events = array ();

				while ($row = DBfetch ($result)) {
					if(($show_unknown == 0) && ($row['value'] == TRIGGER_VALUE_UNKNOWN)) 
						continue;
					if ($start > 0) {
						$start--;
						continue;
					}

					if ($num > 0)
						break;
						
					$res_events[] = $row;
					$num--;
				}
			}
		}
		       
		$table = new CTableInfo(S_NO_EVENTS_FOUND); 
		$table->SetHeader(array(
				S_TIME,
				is_show_subnodes() ? S_NODE : null,
				$hostid == 0 ? S_HOST : null,
				S_DESCRIPTION,
				S_VALUE,
				S_SEVERITY
				));
		
		$accessible_hosts = get_accessible_hosts_by_user($USER_DETAILS,PERM_READ_ONLY);
		
		$col=0;
		$skip = $start;

		foreach ($res_events as $row) {
// 		while(!empty($triggers) && ($col<$num) && ($row=DBfetch($result))){
			
// 			if($skip > 0){
// 				if(($show_unknown == 0) && ($row['value'] == TRIGGER_VALUE_UNKNOWN)) continue;
// 				$skip--;
// 				continue;
// 			}
			
			if($row["value"] == 0)
			{
				$value=new CCol(S_OFF,"off");
			}
			elseif($row["value"] == 1)
			{
				$value=new CCol(S_ON,"on");
			}
			else
			{
				$value=new CCol(S_UNKNOWN_BIG,"unknown");
			}
			
			$row = array_merge($triggers[$row["triggerid"]],$row);
// 			if(($show_unknown == 0) && (!event_initial_time($row,$show_unknown))) continue;
				
			$table->AddRow(array(
				date("Y.M.d H:i:s",$row["clock"]),
				'',
				$hostid == 0 ? $row['host'] : null,
				new CLink(
					expand_trigger_description_by_data($row, ZBX_FLAG_EVENT),
					"tr_events.php?triggerid=".$row["triggerid"],"action"
					),
				$value,
				new CCol(get_severity_description($row["priority"]), get_severity_style($row["priority"]))));
			$col++;
		}
		return $table;
	}

	function	get_history_of_discovery_events($start,$num)
	{
		$db_events = DBselect('select distinct e.source,e.object,e.objectid,e.clock,e.value from events e'.
			' where e.source='.EVENT_SOURCE_DISCOVERY.' order by e.clock desc',
			10*($start+$num)
			);
       
		$table = new CTableInfo(S_NO_EVENTS_FOUND); 
		$table->SetHeader(array(S_TIME, S_IP, S_DESCRIPTION, S_STATUS));
		$col=0;
		
		$skip = $start;
		while(($event_data = DBfetch($db_events))&&($col<$num))
		{
			if($skip > 0) 
			{
				$skip--;
				continue;
			}

			if($event_data["value"] == 0)
			{
				$value=new CCol(S_UP,"off");
			}
			elseif($event_data["value"] == 1)
			{
				$value=new CCol(S_DOWN,"on");
			}
			else
			{
				$value=new CCol(S_UNKNOWN_BIG,"unknown");
			}


			switch($event_data['object'])
			{
				case EVENT_OBJECT_DHOST:
					$object_data = DBfetch(DBselect('select ip from dhosts where dhostid='.$event_data['objectid']));
					$description = SPACE;
					break;
				case EVENT_OBJECT_DSERVICE:
					$object_data = DBfetch(DBselect('select h.ip,s.type,s.port from dhosts h,dservices s '.
						' where h.dhostid=s.dhostid and s.dserviceid='.$event_data['objectid']));
					$description = S_SERVICE.': '.discovery_check_type2str($object_data['type']).'; '.
						S_PORT.': '.$object_data['port'];
					break;
				default:
					continue;
			}

			if(!$object_data) continue;


			$table->AddRow(array(
				date("Y.M.d H:i:s",$event_data["clock"]),
				$object_data['ip'],
				$description,
				$value));

			$col++;
		}
		return $table;
	}
	
/* function:
 *     event_initial_time
 *
 * description:
 *     returs 'true' if event is initial, otherwise false; 
 *
 * author: Aly
 */
function event_initial_time($row,$show_unknown=0){
	$events = get_latest_events($row,$show_unknown);
	
	if(!empty($events) && ($events[0]['value'] == $row['value'])){
		return false;
	}
	return true;
}

function get_latest_events($row,$show_unknown=0){

	$eventz = array();
	$events = array();

// SQL's are optimized that's why it's splited that way	
// func MOD is used on object for forcing MySQL use different Index!!!

/*******************************************/
// Check for optimization after changing!  */
/*******************************************/

	$sql = 'SELECT e.eventid, e.value '.
			' FROM events e '.
			' WHERE e.objectid='.$row['triggerid'].
				' AND e.eventid < '.$row['eventid'].
				' AND '.zbx_sql_mod('e.object',1000).'='.EVENT_OBJECT_TRIGGER.   
				' AND e.value='.TRIGGER_VALUE_FALSE.
			' ORDER BY e.eventid DESC';
	if($rez = DBfetch(DBselect($sql,1))) $eventz[] = $rez['eventid'];
	
	$sql = 'SELECT e.eventid, e.value '.
			' FROM events e'.
			' WHERE e.objectid='.$row['triggerid'].
				' AND e.eventid < '.$row['eventid'].
				' AND '.zbx_sql_mod('e.object',1000).'='.EVENT_OBJECT_TRIGGER.
				' AND e.value='.TRIGGER_VALUE_TRUE.
			' ORDER BY e.eventid DESC';
	if($rez = DBfetch(DBselect($sql,1))) $eventz[] = $rez['eventid'];

	if($show_unknown != 0){
		$sql = 'SELECT e.eventid, e.value '.
				' FROM events e'.
				' WHERE e.objectid='.$row['triggerid'].
					' AND e.eventid < '.$row['eventid'].
					' AND '.zbx_sql_mod('e.object',1000).'='.EVENT_OBJECT_TRIGGER.
					' AND e.value='.TRIGGER_VALUE_UNKNOWN.
				' ORDER BY e.eventid DESC';
		if($rez = DBfetch(DBselect($sql,1))) $eventz[] = $rez['eventid'];
	}

/*******************************************/

	arsort($eventz);
	foreach($eventz as $key => $value){
		$events[] = array('eventid'=>$value,'value'=>$key);
	}
return $events;
}
?>

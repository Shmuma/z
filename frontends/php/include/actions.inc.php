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

include_once 'include/discovery.inc.php';
include_once 'include/hfs.inc.php';

?>
<?php
	function	action_accessiable($actionid,$perm)
	{
		global $USER_DETAILS;

		$result = false;

		if ( DBselect('select actionid from actions where actionid='.$actionid.
			' and '.DBin_node('actionid')) )
		{
			$result = true;
			
			$denyed_hosts = get_accessible_hosts_by_user($USER_DETAILS,PERM_READ_ONLY, PERM_MODE_LT);
			$denyed_groups = get_accessible_groups_by_user($USER_DETAILS,PERM_READ_ONLY, PERM_MODE_LT);
			
			$db_result = DBselect("select * from conditions where actionid=".$actionid);
			while(($ac_data = DBfetch($db_result)) && $result)
			{
				if($ac_data['operator'] != 0) continue;

				switch($ac_data['conditiontype'])
				{
					case CONDITION_TYPE_HOST_GROUP:
						if(in_array($ac_data['value'],explode(',',$denyed_groups)))
						{
							$result = false;
						}
						break;
					case CONDITION_TYPE_HOST:
						if(in_array($ac_data['value'],explode(',',$denyed_hosts)))
						{
							$result = false;
						}
						break;
					case CONDITION_TYPE_TRIGGER:
						if(!DBfetch(DBselect("select distinct t.*".
							" from triggers t,items i,hosts_groups hg,functions f".
							" where f.itemid=i.itemid and t.triggerid=f.triggerid".
							" and i.hostid=hg.hostid and hg.groupid not in (".$denyed_groups.") and t.triggerid=".$ac_data['value'])))
						{
							$result = false;
						}
						break;
				}
			}
		}
		return $result;
	}

	function	check_permission_for_action_conditions($conditions)
	{
		global $USER_DETAILS;

		$result = true;

		$denyed_hosts = get_accessible_hosts_by_user($USER_DETAILS,PERM_READ_ONLY, PERM_MODE_LT);
		$denyed_groups = get_accessible_groups_by_user($USER_DETAILS,PERM_READ_ONLY, PERM_MODE_LT);
		
		foreach($conditions as $ac_data)
		{
			if($ac_data['operator'] != 0) continue;

			switch($ac_data['type'])
			{
				case CONDITION_TYPE_HOST_GROUP:
					if(in_array($ac_data['value'],explode(',',$denyed_groups)))
					{
						error(S_INCORRECT_GROUP);
						$result = false;
					}
					break;
				case CONDITION_TYPE_HOST:
					if(in_array($ac_data['value'],explode(',',$denyed_hosts)))
					{
						error(S_INCORRECT_HOST);
						$result = false;
					}
					break;
				case CONDITION_TYPE_TRIGGER:
					if(!DBfetch(DBselect("select distinct t.*".
						" from triggers t,items i,hosts_groups hg,functions f".
						" where f.itemid=i.itemid and t.triggerid=f.triggerid".
						" and hg.hostid=i.hostid and hg.groupid not in (".$denyed_groups.") and t.triggerid=".$ac_data['value'])))
					{
						error(S_INCORRECT_TRIGGER);
						$result = false;
					}
					break;
			}
			if(!$result) break;
		}
		return $result;
	}

	function	get_action_by_actionid($actionid)
	{
		$sql="select * from actions where actionid=$actionid"; 
		$result=DBselect($sql);
		$row=DBfetch($result);
		if($row)
		{
			return	$row;
		}
		else
		{
			error("No action with actionid=[$actionid]");
		}
		return	$result;
	}

	function get_media_name_by_mediaid ($mediaid)
	{
		$result = DBselect ("select description from media_type where mediatypeid=$mediaid");
		if ($descr = DBfetch ($result))
			return $descr['description'];
		else
			return S_UNKNOWN_MEDIA;
	}

	# Add Action's condition

	function	add_action_condition($actionid, $condition)
	{
		$conditionid = get_dbid("conditions","conditionid");

		$result = DBexecute('insert into conditions (conditionid,actionid,conditiontype,operator,value)'.
			' values ('.$conditionid.','.$actionid.','.
				$condition['type'].','.
				$condition['operator'].','.
				zbx_dbstr($condition['value']).
			')');
		
		if(!$result)
			return $result;

		return $conditionid;
	}

	function	add_action_operation($actionid, $operation)
	{
		$operationid = get_dbid('operations','operationid');

		$result = DBexecute('insert into operations (operationid,actionid,operationtype,object,objectid,objectarg,shortdata,longdata)'.
			' values('.$operationid.','.$actionid.','.
				$operation['operationtype'].','.
				$operation['object'].','.
				$operation['objectid'].','.
				zbx_dbstr($operation['objectarg']).','.
				zbx_dbstr($operation['shortdata']).','.
				zbx_dbstr($operation['longdata']).
			')');
		if(!$result)
			return $result;

		return $operationid;
	}
	# Add Action

	function	add_action($name, $eventsource, $evaltype, $status, $conditions, $operations)
	{
		if(!is_array($conditions) || count($conditions) == 0)
		{
			/*
			error(S_NO_CONDITIONS_DEFINED);
			return false;
			*/
		}
		else
		{
			if(!check_permission_for_action_conditions($conditions))
				return false;

			foreach($conditions as $condition)
				if( !validate_condition($condition['type'], $condition['value']) ) return false;
		}

		if(!is_array($operations) || count($operations) == 0)
		{
			error(S_NO_OPERATIONS_DEFINED);
			return false;
		}

		$new_host_with_group = 0;

		foreach($operations as $operation) {
			if(!validate_operation($operation))
				return false;

			switch ($operation['operationtype']) {
				case OPERATION_TYPE_HOST_ADD:  $new_host_with_group +=1; break;
				case OPERATION_TYPE_GROUP_ADD: $new_host_with_group +=2; break;
			}
		}
		if ($new_host_with_group > 0 && $new_host_with_group < 2) {
			error("You should add host into group");
			return false;
		}

		$actionid=get_dbid("actions","actionid");

		$result = DBexecute('insert into actions (actionid,name,eventsource,evaltype,status)'.
			' values ('.$actionid.','.zbx_dbstr($name).','.$eventsource.','.$evaltype.','.$status.')');

		if(!$result)
			return $result;

		foreach($operations as $operation)
			if( !($result = add_action_operation($actionid, $operation)))
				break;

		if($result)
		{
			foreach($conditions as $condition)
			if( !($result = add_action_condition($actionid, $condition)))
				break;
		}

		if(!$result)
		{
			delete_action($actionid);
			$actionid = $result;
		}

		return $actionid;
	}

	# Update Action

	function	update_action($actionid, $name, $eventsource, $evaltype, $status, $conditions, $operations)
	{
		if(!is_array($conditions) || count($conditions) == 0)
		{
			/*
			error(S_NO_CONDITIONS_DEFINED);
			return false;
			*/
		}
		else
		{
			if(!check_permission_for_action_conditions($conditions))
				return false;

			foreach($conditions as $condition)
				if( !validate_condition($condition['type'],$condition['value']) ) return false;
		}

		if(!is_array($operations) || count($operations) == 0)
		{
			error(S_NO_OPERATIONS_DEFINED);
			return false;
		}

		$new_host_with_group = 0;

		foreach($operations as $operation) {
			if( !validate_operation($operation) )
				return false;

			switch ($operation['operationtype']) {
				case OPERATION_TYPE_HOST_ADD:  $new_host_with_group+=1; break;
				case OPERATION_TYPE_GROUP_ADD: $new_host_with_group+=2; break;
			}
		}
		if ($new_host_with_group > 0 && $new_host_with_group < 2) {
			error("You should add host into group");
			return false;
		}

		$result = DBexecute('update actions set name='.zbx_dbstr($name).',eventsource='.$eventsource.','.
			'evaltype='.$evaltype.',status='.$status.' where actionid='.$actionid);

		if($result)
		{
			DBexecute('delete from conditions where actionid='.$actionid);
			DBexecute('delete from operations where actionid='.$actionid);

			foreach($operations as $operation)
				if( !($result = add_action_operation($actionid, $operation)))
					break;

			if($result)
			{
				foreach($conditions as $condition)
				if( !($result = add_action_condition($actionid, $condition)))
					break;
			}
		}

		return $result;
	}

	# Delete Action

	function	delete_action( $actionid )
	{
		$return = DBexecute('delete from conditions where actionid='.$actionid);

		if($return)
			$result = DBexecute('delete from operations where actionid='.$actionid);

		if($return)
			$result = DBexecute('delete from alerts where actionid='.$actionid);

		if($return)
			$result = DBexecute('delete from actions where actionid='.$actionid);

		return $result;
	}

	function	condition_operator2str($operator)
	{
		$str_op[CONDITION_OPERATOR_EQUAL] 	= '=';
		$str_op[CONDITION_OPERATOR_NOT_EQUAL]	= '<>';
		$str_op[CONDITION_OPERATOR_LIKE]	= S_LIKE_SMALL;
		$str_op[CONDITION_OPERATOR_NOT_LIKE]	= S_NOT_LIKE_SMALL;
		$str_op[CONDITION_OPERATOR_IN]		= S_IN_SMALL;
		$str_op[CONDITION_OPERATOR_MORE_EQUAL]	= '>=';
		$str_op[CONDITION_OPERATOR_LESS_EQUAL]	= '<=';
		$str_op[CONDITION_OPERATOR_NOT_IN]	= S_NOT_IN_SMALL;

		if(isset($str_op[$operator]))
			return $str_op[$operator];

		return S_UNKNOWN;
	}

	function	condition_type2str($conditiontype)
	{
		$str_type[CONDITION_TYPE_HOST_GROUP]		= S_HOST_GROUP;
		$str_type[CONDITION_TYPE_TRIGGER]		= S_TRIGGER;
		$str_type[CONDITION_TYPE_HOST]			= S_HOST;
		$str_type[CONDITION_TYPE_TRIGGER_NAME]		= S_TRIGGER_DESCRIPTION;
		$str_type[CONDITION_TYPE_TRIGGER_VALUE]		= S_TRIGGER_VALUE;
		$str_type[CONDITION_TYPE_TRIGGER_SEVERITY]	= S_TRIGGER_SEVERITY;
		$str_type[CONDITION_TYPE_TIME_PERIOD]		= S_TIME_PERIOD;
		$str_type[CONDITION_TYPE_DHOST_IP]		= S_HOST_IP;
		$str_type[CONDITION_TYPE_DSERVICE_TYPE]		= S_SERVICE_TYPE;
		$str_type[CONDITION_TYPE_DSERVICE_PORT]		= S_SERVICE_PORT;
		$str_type[CONDITION_TYPE_DSTATUS]		= S_DISCOVERY_STATUS;
		$str_type[CONDITION_TYPE_DUPTIME]		= S_UPTIME_DOWNTIME;
		$str_type[CONDITION_TYPE_DVALUE]		= S_RECEIVED_VALUE;

		if(isset($str_type[$conditiontype]))
			return $str_type[$conditiontype];

		return S_UNKNOWN;
	}
		
	function	condition_value2str($conditiontype, $value)
	{
		switch($conditiontype)
		{
			case CONDITION_TYPE_HOST_GROUP:
				$str_val = get_hostgroup_by_groupid($value);
				$str_val = $str_val['name'];
				break;
			case CONDITION_TYPE_TRIGGER:
				$str_val = expand_trigger_description($value);
				break;
			case CONDITION_TYPE_HOST:
				$str_val = get_host_by_hostid($value);
				$str_val = $str_val['host'];
				break;
			case CONDITION_TYPE_TRIGGER_NAME:
				$str_val = $value;
				break;
			case CONDITION_TYPE_TRIGGER_VALUE:
				$str_val = trigger_value2str($value);
				break;
			case CONDITION_TYPE_TRIGGER_SEVERITY:
				$str_val = get_severity_description($value);
				break;
			case CONDITION_TYPE_TIME_PERIOD:
				$str_val = $value;
				break;
			case CONDITION_TYPE_DHOST_IP:
				$str_val = $value;
				break;
			case CONDITION_TYPE_DSERVICE_TYPE:
				$str_val = discovery_check_type2str($value);
				break;
			case CONDITION_TYPE_DSERVICE_PORT:
				$str_val = $value;
				break;
			case CONDITION_TYPE_DSTATUS:
				$str_val = discovery_object_status2str($value);
				break;
			case CONDITION_TYPE_DUPTIME:
				$str_val = $value;
				break;
			case CONDITION_TYPE_DVALUE:
				$str_val = $value;
				break;
			default:
				return S_UNKNOWN;
				break;
		}
		return '"'.$str_val.'"';
	}

	function	get_condition_desc($conditiontype, $operator, $value)
	{
		return condition_type2str($conditiontype).' '.
			condition_operator2str($operator).' '.
			condition_value2str($conditiontype, $value);
	}

	define('LONG_DESCRITION', 0);
	define('SHORT_DESCRITION', 1);
	function get_operation_desc($type=SHORT_DESCRITION, $data)
	{
		$result = null;

		switch($type)
		{
			case SHORT_DESCRITION:
				switch($data['operationtype'])
				{
					case OPERATION_TYPE_MESSAGE:
						switch($data['object'])
						{
							case OPERATION_OBJECT_USER:
								$obj_data = get_user_by_userid($data['objectid']);
								$obj_data = S_USER.' "'.$obj_data['alias'].'"';
								break;
							case OPERATION_OBJECT_GROUP:
								$obj_data = get_group_by_usrgrpid($data['objectid']);
								$obj_data = S_GROUP.' "'.$obj_data['name'].'"';
								break;
						}
						$result = S_SEND_MESSAGE_TO.' '.$obj_data;
						break;
					case OPERATION_TYPE_COMMAND:
						$result = S_RUN_REMOTE_COMMANDS;
						break;
					case OPERATION_TYPE_SEND_TO_MEDIA:
						$result = S_SEND_MESSAGE_TO.' "'.$data['objectarg'].'" '.
							S_USING_MEDIA.' '.get_media_name_by_mediaid ($data['objectid']);
						break;
					case OPERATION_TYPE_HOST_ADD:
						$result = S_ADD_HOST;
						break;
					case OPERATION_TYPE_HOST_REMOVE:
						$result = S_REMOVE_HOST;
						break;
					case OPERATION_TYPE_GROUP_ADD:
						$obj_data = get_hostgroup_by_groupid($data['objectid']);
						$result = S_ADD_TO_GROUP.' "'.$obj_data['name'].'"';
						break;
					case OPERATION_TYPE_GROUP_REMOVE:
						$obj_data = get_hostgroup_by_groupid($data['objectid']);
						$result = S_DELETE_FROM_GROUP.' "'.$obj_data['name'].'"';
						break;
					case OPERATION_TYPE_TEMPLATE_ADD:
						$obj_data = get_host_by_hostid($data['objectid']);
						$result = S_LINK_TO_TEMPLATE.' "'.$obj_data['host'].'"';
						break;
					case OPERATION_TYPE_TEMPLATE_REMOVE:
						$obj_data = get_host_by_hostid($data['objectid']);
						$result = S_UNLINK_FROM_TEMPLATE.' "'.$obj_data['host'].'"';
						break;
					default: break;
				}
				break;
			case LONG_DESCRITION:
				switch($data['operationtype'])
				{
					case OPERATION_TYPE_SEND_TO_MEDIA:
					case OPERATION_TYPE_MESSAGE:
						$result = bold(S_SUBJECT).': '.$data['shortdata']."\n";
						$result .= bold(S_MESSAGE).":\n".$data['longdata'];
						break;
					case OPERATION_TYPE_COMMAND:
						$result = bold(S_REMOTE_COMMANDS).":\n".$data['longdata'];
						break;
					default: break;
				}
				break;
			default:
				break;
		}

		return $result;
	}

	function	get_conditions_by_eventsource($eventsource)
	{
		$conditions[EVENT_SOURCE_TRIGGERS] = array(
				CONDITION_TYPE_HOST_GROUP,
				CONDITION_TYPE_HOST,
				CONDITION_TYPE_TRIGGER,
				CONDITION_TYPE_TRIGGER_NAME,
				CONDITION_TYPE_TRIGGER_SEVERITY,
				CONDITION_TYPE_TRIGGER_VALUE,
				CONDITION_TYPE_TIME_PERIOD
			);
		$conditions[EVENT_SOURCE_DISCOVERY] = array(
				CONDITION_TYPE_DHOST_IP,
				CONDITION_TYPE_DSERVICE_TYPE,
				CONDITION_TYPE_DSERVICE_PORT,
				CONDITION_TYPE_DSTATUS,
				CONDITION_TYPE_DUPTIME,
				CONDITION_TYPE_DVALUE
			);

		if(isset($conditions[$eventsource]))
			return $conditions[$eventsource];

		return $conditions[EVENT_SOURCE_TRIGGERS];
	}

	function	get_operations_by_eventsource($eventsource)
	{
		$operations[EVENT_SOURCE_TRIGGERS] = array(
				OPERATION_TYPE_MESSAGE,
				OPERATION_TYPE_COMMAND,
				OPERATION_TYPE_SEND_TO_MEDIA
			);
		$operations[EVENT_SOURCE_DISCOVERY] = array(
				OPERATION_TYPE_MESSAGE,
				OPERATION_TYPE_COMMAND,
				OPERATION_TYPE_HOST_ADD,
				OPERATION_TYPE_HOST_REMOVE,
				OPERATION_TYPE_GROUP_ADD,
				OPERATION_TYPE_GROUP_REMOVE,
				OPERATION_TYPE_TEMPLATE_ADD,
				OPERATION_TYPE_TEMPLATE_REMOVE
			);

		if(isset($operations[$eventsource]))
			return $operations[$eventsource];

		return $operations[EVENT_SOURCE_TRIGGERS];
	}

	function	operation_type2str($type)
	{
		$str_type[OPERATION_TYPE_MESSAGE]		= S_SEND_MESSAGE;
		$str_type[OPERATION_TYPE_COMMAND]		= S_REMOTE_COMMAND;
		$str_type[OPERATION_TYPE_HOST_ADD]		= S_ADD_HOST;
		$str_type[OPERATION_TYPE_HOST_REMOVE]		= S_REMOVE_HOST;
		$str_type[OPERATION_TYPE_GROUP_ADD]		= S_ADD_TO_GROUP;
		$str_type[OPERATION_TYPE_GROUP_REMOVE]		= S_DELETE_FROM_GROUP;
		$str_type[OPERATION_TYPE_TEMPLATE_ADD]		= S_LINK_TO_TEMPLATE;
		$str_type[OPERATION_TYPE_TEMPLATE_REMOVE]	= S_UNLINK_FROM_TEMPLATE;
		$str_type[OPERATION_TYPE_SEND_TO_MEDIA]		= S_SEND_TO_MEDIA;

		if(isset($str_type[$type]))
			return $str_type[$type];

		return S_UNKNOWN;
	}

	function	get_operators_by_conditiontype($conditiontype)
	{
		$operators[CONDITION_TYPE_HOST_GROUP] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL
			);
		$operators[CONDITION_TYPE_HOST] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL
			);
		$operators[CONDITION_TYPE_TRIGGER] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL
			);
		$operators[CONDITION_TYPE_TRIGGER_NAME] = array(
				CONDITION_OPERATOR_LIKE,
				CONDITION_OPERATOR_NOT_LIKE	
			);
		$operators[CONDITION_TYPE_TRIGGER_SEVERITY] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL,
				CONDITION_OPERATOR_MORE_EQUAL,
				CONDITION_OPERATOR_LESS_EQUAL
			);
		$operators[CONDITION_TYPE_TRIGGER_VALUE] = array(
				CONDITION_OPERATOR_EQUAL
			);
		$operators[CONDITION_TYPE_TIME_PERIOD] = array(
				CONDITION_OPERATOR_IN,
				CONDITION_OPERATOR_NOT_IN
			);
		$operators[CONDITION_TYPE_DHOST_IP] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL
			);
		$operators[CONDITION_TYPE_DSERVICE_TYPE] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL
			);
		$operators[CONDITION_TYPE_DSERVICE_PORT] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL
			);
		$operators[CONDITION_TYPE_DSTATUS] = array(
				CONDITION_OPERATOR_EQUAL,
			);
		$operators[CONDITION_TYPE_DUPTIME] = array(
				CONDITION_OPERATOR_MORE_EQUAL,
				CONDITION_OPERATOR_LESS_EQUAL
			);
		$operators[CONDITION_TYPE_DVALUE] = array(
				CONDITION_OPERATOR_EQUAL,
				CONDITION_OPERATOR_NOT_EQUAL,
				CONDITION_OPERATOR_MORE_EQUAL,
				CONDITION_OPERATOR_LESS_EQUAL,
				CONDITION_OPERATOR_LIKE,
				CONDITION_OPERATOR_NOT_LIKE
			);

		if(isset($operators[$conditiontype]))
			return $operators[$conditiontype];

		return array();
	}

	function	update_action_status($actionid, $status)
	{
		return DBexecute("update actions set status=$status where actionid=$actionid");
	}

	function validate_condition($conditiontype, $value)
	{
		global $USER_DETAILS;

		switch($conditiontype)
		{
			case CONDITION_TYPE_HOST_GROUP:
				if(!in_array($value,
					get_accessible_groups_by_user($USER_DETAILS,PERM_READ_ONLY,null,
						PERM_RES_IDS_ARRAY)))
				{
					error(S_INCORRECT_GROUP);
					return false;
				}
				break;
			case CONDITION_TYPE_TRIGGER:
				if( !DBfetch(DBselect('select triggerid from triggers where triggerid='.$value)) || 
					!check_right_on_trigger_by_triggerid(PERM_READ_ONLY, $value) )
				{
					error(S_INCORRECT_TRIGGER);
					return false;
				}
				break;
			case CONDITION_TYPE_HOST:
				if(!in_array($value,
					get_accessible_hosts_by_user($USER_DETAILS,PERM_READ_ONLY,null,
						PERM_RES_IDS_ARRAY)))
				{
					error(S_INCORRECT_HOST);
					return false;
				}
				break;
			case CONDITION_TYPE_TIME_PERIOD:
				if( !validate_period($value) )
				{
					error(S_INCORRECT_PERIOD.' ['.$value.']');
					return false;
				}
				break;
			case CONDITION_TYPE_DHOST_IP:
				if( !validate_ip_range($value) )
				{
					error(S_INCORRECT_IP.' ['.$value.']');
					return false;
				}
				break;
			case CONDITION_TYPE_DSERVICE_TYPE:
				if( S_UNKNOWN == discovery_check_type2str($value) )
				{
					error(S_INCORRECT_DISCOVERY_CHECK);
					return false;
				}
				break;
			case CONDITION_TYPE_DSERVICE_PORT:
				if( !validate_port_list($value) )
				{
					error(S_INCORRECT_PORT.' ['.$value.']');
					return false;
				}
				break;
			case CONDITION_TYPE_DSTATUS:
				if( S_UNKNOWN == discovery_status2str($value) )
				{
					error(S_INCORRECT_DISCOVERY_STATUS);
					return false;
				}
				break;
			case CONDITION_TYPE_TRIGGER_NAME:
			case CONDITION_TYPE_TRIGGER_VALUE:
			case CONDITION_TYPE_TRIGGER_SEVERITY:
			case CONDITION_TYPE_DUPTIME:
			case CONDITION_TYPE_DVALUE:
				break;
			default:
				error(S_INCORRECT_CONDITION_TYPE);
				return false;
				break;
		}
		return true;
	}

	function	validate_operation($operation)
	{
		global $USER_DETAILS;

		switch($operation['operationtype'])
		{
			case OPERATION_TYPE_MESSAGE:
				switch($operation['object'])
				{
					case OPERATION_OBJECT_USER:
						if( !get_user_by_userid($operation['objectid']) )
						{
							error(S_INCORRECT_USER);
							return false;
						}
						break;
					case OPERATION_OBJECT_GROUP:
						if( !get_group_by_usrgrpid($operation['objectid']) )
						{
							error(S_INCORRECT_GROUP);
							return false;
						}
						break;
					default:
						error(S_INCORRECT_OBJECT_TYPE);
						return false;
				}
				break;
			case OPERATION_TYPE_COMMAND:
				return validate_commands($operation['longdata']);
			case OPERATION_TYPE_SEND_TO_MEDIA:
				break;
			case OPERATION_TYPE_HOST_ADD:
			case OPERATION_TYPE_HOST_REMOVE:
				break;
			case OPERATION_TYPE_GROUP_ADD:
			case OPERATION_TYPE_GROUP_REMOVE:
				if(!in_array($operation['objectid'],
					get_accessible_groups_by_user($USER_DETAILS,PERM_READ_WRITE,null,
						PERM_RES_IDS_ARRAY)))
				{
					error(S_INCORRECT_GROUP);
					return false;
				}
				break;
			case OPERATION_TYPE_TEMPLATE_ADD:
			case OPERATION_TYPE_TEMPLATE_REMOVE:
				if(!in_array($operation['objectid'],
					get_accessible_hosts_by_user($USER_DETAILS,PERM_READ_WRITE,null,
						PERM_RES_IDS_ARRAY)))
				{
					error(S_INCORRECT_HOST);
					return false;
				}
				break;
			default:
				error(S_INCORRECT_OPERATION_TYPE);
				return false;
		}
		return true;
	}

	function validate_commands($commands)
	{
		$cmd_list = split("\n",$commands);
		foreach($cmd_list as $cmd)
		{
			$cmd = trim($cmd, "\x00..\x1F");
			if(!ereg("^(({HOSTNAME})|([0-9a-zA-Z\_\.[.-.]]{1,}))(:|#)[[:print:]]*$",$cmd,$cmd_items)){
				error("Incorrect command: '$cmd'");
				return FALSE;
			}
			if($cmd_items[4] == "#")
			{ // group
				if(!DBfetch(DBselect("select groupid from groups where name=".zbx_dbstr($cmd_items[1]))))
				{
					error("Unknown group name: '".$cmd_items[1]."' in command ".$cmd."'");
					return FALSE;
				}
			}
			elseif($cmd_items[4] == ":")
			{ // host
				if( $cmd_items[1] != '{HOSTNAME}' && 
					!DBfetch(DBselect("select hostid from hosts where host=".zbx_dbstr($cmd_items[1]))) )
				{
					error("Unknown host name '".$cmd_items[1]."' in command '".$cmd."'");
					return FALSE;
				}
			}
		}
		return TRUE;
	}


	function alerts_sort ($a, $b)
	{
		return $b->clock - $a->clock;
	}


	function alerts_filter_by_user ($a)
	{
		global $USER_DETAILS;
		return $a->userid == $USER_DETAILS["userid"];
	}

	function get_history_of_actions($start,$num)
	{
		global $USER_DETAILS;
		
		$res_alerts = array ();
		if (zbx_hfs_available ()) {
			$stop = $count = 0;

			foreach (zbx_hfs_sites (0, 0) as $site) {
				$site_stat[$site] = new stdClass ();
				$site_stat[$site]->name  = $site;
				$site_stat[$site]->begin = 0;
				$site_stat[$site]->stop  = 0;
			}

			// prepare mediatype hash
			$result = DBselect ("select mediatypeid, description from media_type");
			$medias = array ();
			while ($row = DBfetch ($result))
				$medias[$row["mediatypeid"]] = $row["description"];

			// we have sites list. Ok, feeding alerts array
			$orig_num = $num;
			while (!$stop && $num > 0) {
				$stop = 1;
				$alerts = array ();
				foreach ($site_stat as $site) {
					if ($site->stop)
						continue;
					$stop = 0;
					$hfs_alerts = zabbix_hfs_get_alerts ($site->name, $site->begin, $orig_num);
					if (is_array ($hfs_alerts) && count ($hfs_alerts) > 0) {
						//						
// 						print "<pre>$orig_num, ".count($hfs_alerts)."</pre>";
						if (count ($hfs_alerts) < $orig_num)
							$site->stop = 1;
						$site->begin += count ($hfs_alerts);
						if ($USER_DETAILS["type"] < USER_TYPE_SUPER_ADMIN)
							$hfs_alerts = array_filter ($hfs_alerts, "alerts_filter_by_user");
						$alerts = array_merge ($alerts, $hfs_alerts);
// 						print "<pre>$site->name, $site->begin, $site->stop, ".count ($hfs_alerts)."</pre>";
// 						print "<pre>";
// 						print_r ($hfs_alerts);
// 						print "</pre>";
					}
					else 
						$site->stop = 1;
				}

				if ($stop)
					break;

				usort ($alerts, "alerts_sort");
				$len = count ($alerts);
				
// 				print "<pre>len = $len, num = $num, start = $start, res_count = ".count ($res_alerts)."</pre>";
// 				print "<pre>";
// 				print_r ($site_stat);
// 				print "</pre>\n";
				if ($len == 0)
					break;
				if ($start > 0) {
					if ($len <= $start) {
						$start -= $len;
						$alerts = array ();
					} else {
						array_splice ($alerts, 0, $start);
						$start = 0;
					}
					$len = count ($alerts);
				}
				else {
					if ($len > $num) {
						array_splice ($alerts, $num);
						$len = count ($alerts);
					}
					$res_alerts = array_merge ($res_alerts, $alerts);
					$num -= $len;
				}
			}
		}
		else {
			$denyed_groups = get_accessible_groups_by_user($USER_DETAILS, PERM_READ_ONLY, PERM_MODE_LT);
		
			$result=DBselect("select distinct a.alertid,a.clock,mt.description,a.sendto,a.subject,a.message,a.status,a.retries,".
					"a.error from alerts a,media_type mt,functions f,items i,hosts_groups hg ".
					" where mt.mediatypeid=a.mediatypeid and a.triggerid=f.triggerid and f.itemid=i.itemid ".
					" and hg.hostid=i.hostid and hg.groupid not in (".$denyed_groups.")".
					' and '.DBin_node('a.alertid').
					" order by a.clock".
					" desc", 10*$start+$num);

			while ($row = DBfetch ($result)) {
				if ($start > 0) {
					$start--;
					continue;
				}

				if ($num == 0)
					break;

				$res_alerts[] = $row;
				$num--;
			}
		}

// 		print "<pre>res_count = ".count ($res_alerts)."</pre>";
		usort ($res_alerts, "alerts_sort");
		$table = new CTableInfo(S_NO_ACTIONS_FOUND);
		$table->SetHeader(array(
				is_show_subnodes() ? S_NODES : null,
				S_TIME,
				S_TYPE,
				S_STATUS,
				S_RETRIES_LEFT,
				S_RECIPIENTS,
				S_MESSAGE
				));
		$col=0;
		foreach ($res_alerts as $row) 
		{
			if (zbx_hfs_available ())
				$row->description = $medias[$row->mediatypeid];
			$time=date("Y.M.d H:i:s",$row->clock);

			if($row->status == ALERT_STATUS_SENT)
			{
				$status=new CSpan(S_SENT,"off");
				$retries=new CSpan(SPACE,"off");
			}
			else
			{
				$status=new CSpan(S_NOT_SENT,"on");
				$retries=new CSpan(3 - $row->retries,"on");
			}
			$sendto=htmlspecialchars($row->sendto);

// 			$subject = empty($row->subject) ? '' : "<pre>".bold(S_SUBJECT.': ').htmlspecialchars($row->subject)."</pre>";
			$subject = '';
			$message = array($subject,"<pre>".htmlspecialchars($row->message)."</pre>");

			$table->AddRow(array(
				new CCol($time, 'top'),
				new CCol($row->description, 'top'),
				new CCol($status, 'top'),
				new CCol($retries, 'top'),
				new CCol($sendto, 'top'),
				new CCol($message, 'top')));
		}

		return $table;
	}
?>

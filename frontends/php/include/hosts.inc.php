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
require_once "include/graphs.inc.php";
require_once "include/profiles.inc.php";
require_once "include/triggers.inc.php";
require_once "include/items.inc.php";

/* SITES functions */
	function	add_site($name, $descr, $dburl, $active)
	{
		return db_save_site($name, $descr, $dburl, $active);
	}

	function	update_site($siteid, $name, $descr, $dburl, $active)
	{
		return db_save_site($name, $descr, $dburl, $active, $siteid);
	}

	function	delete_site($siteid)
	{
		$db_count = DBselect ("select count(*) as hosts from hosts where siteid = $siteid");
		$count = DBfetch ($db_count);
		$site = get_site_by_siteid ($siteid);
		if ($count['hosts'] > 0)
		{
			error ("Cannot delete site '".$site["name"]."' it has associated nodes");
			return false;
		}

		return DBexecute("delete from sites where siteid=$siteid");
	}

	/*
	 * Function: db_save_site
	 *
	 * Description:
	 *     Add new or update site
	 *
	 * Author:
	 *     Max Lapan (max.lapan@gmail.com)
	 *
	 * Comments:
	 *
	 */
	function	db_save_site($name, $descr, $dburl, $active, $siteid=null)
	{
		if(!is_string($name) || !is_string($descr)){
			error("incorrect parameters for 'db_save_site'");
			return false;
		}

		if($siteid==null)
			$result = DBselect("select * from sites where name=".zbx_dbstr($name));
		else
			$result = DBselect("select * from sites where name=".zbx_dbstr($name).
				" and siteid<>$siteid");

		if(DBfetch($result))
		{
			error("Site '$name' already exists");
			return false;
		}
		if($siteid==null)
		{
			$siteid=get_dbid("sites","siteid");
			if(!DBexecute("insert into sites (siteid,name,description, db_url, active) values (".$siteid.",".zbx_dbstr($name).",".zbx_dbstr($descr).",".zbx_dbstr($dburl).",".$active.")"))
				return false;
			return $siteid;

		}
		else
			return DBexecute("update sites set name=".zbx_dbstr($name).", description=".zbx_dbstr($descr).", db_url=".zbx_dbstr($dburl).", active=$active where siteid=$siteid");
	}


/* HOST GROUP functions */
	function	add_host_to_group($hostid, $groupid)
	{
		if(!is_numeric($hostid) || !is_numeric($groupid)){
			error("incorrect parameters for 'add_host_to_group' [hostid:".$hostid."][groupid:".$groupid."]");
			return false;
		}
		$hostgroupid=get_dbid("hosts_groups","hostgroupid");
		$result=DBexecute("insert into hosts_groups (hostgroupid,hostid,groupid) values ($hostgroupid,$hostid,$groupid)");
		if(!$result)
			return $result;
		return $hostgroupid;
	}

	function	delete_host_from_group($hostid, $groupid)
	{
		if(!is_numeric($hostid) || !is_numeric($groupid)){
			error("incorrect parameters for 'add_host_to_group' [hostid:".$hostid."][groupid:".$groupid."]");
			return false;
		}
		return DBexecute('delete from hosts_groups where hostid='.$hostid.' and groupid='.$groupid);
	}

	/*
	 * Function: db_save_group
	 *
	 * Description:
	 *     Add new or update host group
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 */
	function	db_save_group($name,$groupid=null)
	{
		if(!is_string($name)){
			error("incorrect parameters for 'db_save_group'");
			return false;
		}
	
		if($groupid==null)
			$result = DBselect("select * from groups where name=".zbx_dbstr($name));
		else
			$result = DBselect("select * from groups where name=".zbx_dbstr($name).
				" and groupid<>$groupid");
		
		if(DBfetch($result))
		{
			error("Group '$name' already exists");
			return false;
		}
		if($groupid==null)
		{
			$groupid=get_dbid("groups","groupid");
			if(!DBexecute("insert into groups (groupid,name) values (".$groupid.",".zbx_dbstr($name).")"))
				return false;
			return $groupid;

		}
		else
			return DBexecute("update groups set name=".zbx_dbstr($name)." where groupid=$groupid");
	}

	function	add_group_to_host($hostid,$newgroup="")
	{
		if($newgroup == "" || $newgroup == null)
			 return true;

		$groupid = db_save_group($newgroup);
		if(!$groupid)
			return	$groupid;
		
		return add_host_to_group($hostid, $groupid);
	}

	function	update_host_groups_by_groupid($groupid,$hosts=array())
	{
		DBexecute("delete from hosts_groups where groupid=$groupid");

		foreach($hosts as $hostid)
		{
			add_host_to_group($hostid, $groupid);
		}
	}

	function	update_host_groups($hostid,$groups=array())
	{
		DBexecute("delete from hosts_groups where hostid=$hostid");

		foreach($groups as $groupid)
		{
			add_host_to_group($hostid, $groupid);
		}
	}

	function	add_host_group($name,$hosts=array())
	{
		$groupid = db_save_group($name);
		if(!$groupid)
			return	$groupid;
		
		update_host_groups_by_groupid($groupid,$hosts);

		return $groupid;
	}

	function	update_host_group($groupid,$name,$hosts)
	{
		$result = db_save_group($name,$groupid);
		if(!$result)
			return	$result;
		
		update_host_groups_by_groupid($groupid,$hosts);

		return $result;
	}


	function get_template_id ($id)
	{
		if (ereg ("([0-9]+)_([0-9]+)", $id, $reg))
			return $reg[1];
		else
			return $id;
	}


	function get_template_params ($name)
	{
		if (ereg (".+\[(.*)\]", $name, $reg))
			return $reg[1];
		else
			return $name;
	}

	/*
	 * Function: check_circle_host_link
	 *
	 * Description:
	 *     Check templates linage circeling
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 *     NOTE: templates = array(id => name, id2 => name2, ...)
	 *
	 */
	function 	check_circle_host_link($hostid, $templates)
	{
		if(count($templates) == 0)	return false;
		if(isset($templates[$hostid]))	return true;
		foreach($templates as $id => $name)
			if(check_circle_host_link($hostid, get_templates_by_hostid(get_template_id ($id))))
				return true;
			
		return false;
	}

	/*
	 * Function: db_save_host
	 *
	 * Description:
	 *     Add or update host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *     if hostid is NULL add new host, in other cases update
	 *
	 *     NOTE: templates = array(id => name, id2 => name2, ...)
	 */
	function	db_save_host($host,$port,$status,$useip,$dns,$ip,$siteid,$templates,$hostid=null)
	{
		if( !eregi('^'.ZBX_EREG_HOST_FORMAT.'$', $host) )
		{
			error("Incorrect characters used for Hostname");
			return false;
		}

 		if ( !empty($dns) && !eregi('^'.ZBX_EREG_DNS_FORMAT.'$', $dns)) 
		{
			error("Incorrect characters used for DNS");
			return false;
		}


		if(DBfetch(DBselect(
			"select * from hosts where host=".zbx_dbstr($host).
				' and '.DBin_node('hostid', get_current_nodeid(false)).
				(isset($hostid) ? ' and hostid<>'.$hostid : '')
			)))
		{
			error("Host '$host' already exists");
			return false;
		}

		if($hostid==null)
		{
			$hostid = get_dbid("hosts","hostid");
			$result = DBexecute("insert into hosts".
				" (hostid,host,port,status,useip,dns,ip,disable_until,available,siteid)".
				" values ($hostid,".zbx_dbstr($host).",$port,$status,$useip,".zbx_dbstr($dns).",".zbx_dbstr($ip).",0,"
				.HOST_AVAILABLE_UNKNOWN.",$siteid)");
		}
		else
		{
			if(check_circle_host_link($hostid, $templates))
			{
				error("Circle link can't be created");
				return false;
			}

			if (!($result = DBexecute("update hosts set host=".zbx_dbstr($host).",".
				"port=$port,useip=$useip,dns=".zbx_dbstr($dns).",ip=".zbx_dbstr($ip).",siteid=$siteid where hostid=$hostid")))
			{
				error("Unable to update host '$host'");
				return false;
			}

			if (!($result = DBexecute("update items set siteid=$siteid where hostid=$hostid")))
			{
				error("Unable to update items for '$host'");
				return false;
			}

			update_host_status($hostid, $status);
		}
		
		foreach($templates as $id => $name)
		{
			$hosttemplateid = get_dbid('hosts_templates', 'hosttemplateid');
			if(!($result = DBexecute('insert into hosts_templates values ('.$hosttemplateid.','.
						 $hostid.','.get_template_id ($id).','.zbx_dbstr (get_template_params ($name)).')')))
				break;
		}

		if($result) $result = $hostid;
		
		return $result;
	}

	/*
	 * Function: add_host
	 *
	 * Description:
	 *     Add new  host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 *     NOTE: templates = array(id => name, id2 => name2, ...)
	 */
	function	add_host($host,$port,$status,$useip,$dns,$ip,$siteid,$templates,$newgroup,$groups)
	{
		$hostid = db_save_host($host,$port,$status,$useip,$dns,$ip,$siteid,$templates);
		if(!$hostid)
			return $hostid;
		else
			info('Added new host ['.$host.']');

		update_host_groups($hostid,$groups);

		add_group_to_host($hostid,$newgroup);

		sync_host_with_templates($hostid);

		update_profile("HOST_PORT",$port);
		
		return	$hostid;
	}

	/*
	 * Function: update_host
	 *
	 * Description:
	 *     Update host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 *     NOTE: templates = array(id => name, id2 => name2, ...)
	 */
	function	update_host($hostid,$host,$port,$status,$useip,$dns,$ip,$siteid,$templates,$newgroup,$groups)
	{
		// filter parametrized templates from $templates array
		

		$old_templates = get_templates_by_hostid($hostid);
		$unlinked_templates = array_diff($old_templates, $templates);
		foreach($unlinked_templates as $id => $name)
		{
			unlink_template($hostid, $id);
		}
		
		$old_host = get_host_by_hostid($hostid);

		$new_templates = array_diff($templates, $old_templates);

		$result = db_save_host($host,$port,$status,$useip,$dns,$ip,$siteid,$new_templates,$hostid);
		if(!$result)
			return $result;

		update_host_groups($hostid, $groups);

		add_group_to_host($hostid,$newgroup);

		if(count($new_templates) > 0)
		{
			sync_host_with_templates($hostid,array_keys($new_templates));
		}

		return	$result;
	}

	/*
	 * Function: unlink_template
	 *
	 * Description:
	 *     Unlink elements from host by template
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 */
	function	unlink_template($hostid, $templateid, $unlink_mode = true)
	{
		if( !is_numeric($templateid) ) fatal_error('Not supported type for [templateid] in [unlink_template] - ['.$templateid.']');

		delete_template_elements($hostid, $templateid, $unlink_mode);
		DBexecute("delete from hosts_templates where hostid=".$hostid.' and templateid='.$templateid);
	}

	/*
	 * Function: delete_template_elements
	 *
	 * Description:
	 *     Delete all elements from host by template
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 */
	function	delete_template_elements($hostid, $templateid = null, $unlink_mode = false)
	{
		delete_template_graphs($hostid, $templateid, $unlink_mode);
		delete_template_triggers($hostid, $templateid, $unlink_mode);
		delete_template_items($hostid, $templateid, $unlink_mode);
		delete_template_applications($hostid, $templateid, $unlink_mode);
	}	

	/*
	 * Function: copy_template_elements 
	 *
	 * Description:
	 *     Copy all elements from template to host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 */
	function	copy_template_elements($hostid, $templateid = null, $copy_mode = false)
	{
		copy_template_applications($hostid, $templateid, $copy_mode);
		copy_template_items($hostid, $templateid, $copy_mode);
		copy_template_triggers($hostid, $templateid, $copy_mode);
		copy_template_graphs($hostid, $templateid, $copy_mode);
	}

	/*
	 * Function: sync_host_with_templates
	 *
	 * Description:
	 *     Synchronize template elements with host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 */
	function	sync_host_with_templates($hostid, $templateid = null)
	{
		delete_template_elements($hostid, $templateid);		
		copy_template_elements($hostid, $templateid);
	}

	function	delete_groups_by_hostid($hostid)
	{
		$sql="select groupid from hosts_groups where hostid=$hostid";
		$result=DBselect($sql);
		while($row=DBfetch($result))
		{
			$sql="delete from hosts_groups where hostid=$hostid and groupid=".$row["groupid"];
			DBexecute($sql);
			$sql="select count(*) as count from hosts_groups where groupid=".$row["groupid"];
			$result2=DBselect($sql);
			$row2=DBfetch($result2);
			if($row2["count"]==0)
			{
				delete_host_group($row["groupid"]);
			}
		}
	}

	/*
	 * Function: delete_host
	 *
	 * Description:
	 *     Delete host with all elements and relations
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 */
	function	delete_host($hostid, $unlink_mode = false)
	{
		global $DB_TYPE;

		$ret = false;

	// unlink child hosts
		$db_childs = get_hosts_by_templateid($hostid);
		while($db_child = DBfetch($db_childs))
		{
			unlink_template($db_child["hostid"], $hostid, $unlink_mode);
		}

	// delete items -> triggers -> graphs
		$db_items = get_items_by_hostid($hostid);
		while($db_item = DBfetch($db_items))
		{
			delete_item($db_item["itemid"]);
		}

	// delete host from maps
		delete_sysmaps_elements_with_hostid($hostid);
		
	// delete host from group
		DBexecute("delete from hosts_groups where hostid=$hostid");

	// delete host from template linkages
		DBexecute("delete from hosts_templates where hostid=$hostid");

	// disable actions
		$db_actions = DBselect("select distinct actionid from conditions ".
				       " where conditiontype=".CONDITION_TYPE_HOST." and value=".zbx_dbstr ($hostid));
		while($db_action = DBfetch($db_actions))
		{
			DBexecute("update actions set status=".ACTION_STATUS_DISABLED.
				" where actionid=".$db_action["actionid"]);
		}
	// delete action conditions
		DBexecute('delete from conditions where conditiontype='.CONDITION_TYPE_HOST.' and value='.zbx_dbstr ($hostid));

	// delete host profile
		delete_host_profile($hostid);

	// delete host
		return DBexecute("delete from hosts where hostid=$hostid");
	}

	function	delete_host_group($groupid)
	{
		if(!delete_sysmaps_elements_with_groupid($groupid))
			return false;

		if(!DBexecute("delete from rights where id=".$groupid))
			return false;
		
		if(!DBexecute("delete from hosts_groups where groupid=$groupid"))
			return false;

		return DBexecute("delete from groups where groupid=$groupid");
	}

	function	get_hostgroup_by_groupid($groupid)
	{
		$result=DBselect("select * from groups where groupid=".$groupid);
		$row=DBfetch($result);
		if($row)
		{
			return $row;
		}
		error("No host groups with groupid=[$groupid]");
		return  false;
	}

	function	get_site_by_siteid($siteid)
	{
		$result=DBselect("select * from sites where siteid=".$siteid);
		$row=DBfetch($result);
		if($row)
		{
			return $row;
		}
		error("No sites with siteid=[$siteid]");
		return  false;
	}

	function	get_host_by_itemid($itemid)
	{
		$sql="select h.* from hosts h, items i where i.hostid=h.hostid and i.itemid=$itemid";
		$result=DBselect($sql);
		$row=DBfetch($result);
		if($row)
		{
			return $row;
		}
		error("No host with itemid=[$itemid]");
		return	false;
	}

	function	get_host_by_hostid($hostid,$no_error_message=0)
	{
		$sql="select * from hosts where hostid=$hostid";
		$result=DBselect($sql);
		$row=DBfetch($result);
		if($row)
		{
			return $row;
		}
		if($no_error_message == 0)
			error("No host with hostid=[$hostid]");
		return	false;
	}

	function	&get_hosts_by_templateid($templateid)
	{
		return DBselect("select h.* from hosts h, hosts_templates ht where h.hostid=ht.hostid and ht.templateid=$templateid");
	}

	# Update Host status

	function	update_host_status($hostid,$status)
	{
		$row=DBfetch(DBselect("select status,host from hosts where hostid=$hostid"));
		$old_status=$row["status"];
		if($status != $old_status)
		{
			update_trigger_value_to_unknown_by_hostid($hostid);
			info("Updated status of host ".$row["host"]);
			return	DBexecute("update hosts set status=$status".
				" where hostid=$hostid and status!=".HOST_STATUS_DELETED);
		}
		else
		{
			return 1;
		}
	}
	
	/*
	 * Function: get_templates_by_hostid
	 *
	 * Description:
	 *     Retrive templates for specified host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 */
	function	get_templates_by_hostid($hostid)
	{
		$resuilt = array();
		$db_templates = DBselect('select distinct h.hostid,h.host,ht.params from hosts_templates ht '.
			' left join hosts h on h.hostid=ht.templateid '.
			' where ht.hostid='.$hostid);
		while($template_data = DBfetch($db_templates))
		{
			$name = $template_data['host'];
			if (!empty ($template_data['params']))
				$name .= '['.$template_data['params'].']';
			$resuilt[$template_data['hostid']] = $name;
		}
		return $resuilt;
	}

	/*
	 * Function: get_correct_group_and_host
	 *
	 * Description:
	 *     Retrive correct relations for group and host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 */
	function get_correct_group_and_host($a_groupid=null, $a_hostid=null, $perm=PERM_READ_WRITE, $options = array())
	{
		if(!is_array($options))
		{
			fatal_error("Incorrest options for get_correct_group_and_host");
		}
		
		global $USER_DETAILS;
		
		$first_hostid_in_group = 0;

		$allow_all_hosts = (in_array("allow_all_hosts",$options)) ? 1 : 0;
		$always_select_first_host = in_array("always_select_first_host",$options) ? 1 : 0;
		$only_current_node = in_array("only_current_node",$options) ? 1 : 0;

		if(in_array("monitored_hosts",$options))
			$with_host_status = " and h.status=".HOST_STATUS_MONITORED;
		elseif(in_array('real_hosts',$options))
			$with_host_status = " and h.status<>".HOST_STATUS_TEMPLATE;
		else
			$with_host_status = "";

		if(in_array("with_monitored_items",$options)){
			$item_table = ",items i";	$with_items = " and h.hostid=i.hostid and i.status=".ITEM_STATUS_ACTIVE;
		}else if(in_array("with_items",$options)){
			$item_table = ",items i";	$with_items = " and h.hostid=i.hostid";
		} else {
			$item_table = "";
			$with_items = "";
		}

		$with_node = "";

// 		$accessed_hosts = get_accessible_hosts_by_user($USER_DETAILS,$perm,null,null,get_current_nodeid(!$only_current_node));
 		$accessed_groups = get_accessible_groups_by_user($USER_DETAILS,$perm,null,null,get_current_nodeid(!$only_current_node));

//SDI(get_current_nodeid(!$only_current_node));
//SDI($accessed_hosts);

		if(is_null($a_groupid))
		{
			$groupid = 0;
		}
		else
		{
			$groupid = $a_groupid;

			if($groupid > 0)
			{
				$with_node = " and ".DBin_node('g.groupid', get_current_nodeid(!$only_current_node));

				if(!DBfetch(DBselect("select distinct g.groupid from groups g, hosts_groups hg, hosts h".$item_table.
						     " where hg.groupid=g.groupid and h.hostid=hg.hostid and hg.groupid in (".$accessed_groups.") ".
					" and g.groupid=".$groupid.$with_host_status.$with_items.$with_node)))
				{
					$groupid = 0;
				}
			}

		}

		if(is_null($a_hostid))
		{
			$hostid = 0;
		}
		else
		{
			$hostid = $a_hostid;
			if(!($hostid == 0 && $allow_all_hosts == 1)) /* is not 'All' selected */
			{
				$group_table = "";
				$witth_group = "";

				if($groupid != 0)
				{
					$with_node = " and ".DBin_node('hg.hostid', get_current_nodeid(!$only_current_node));
					
					if(!DBfetch(DBselect("select hg.hostid from hosts_groups hg".
						" where hg.groupid=".$groupid." and hg.hostid=".$hostid.$with_node)))
					{
						$hostid = 0;
					}
					$witth_group = " and hg.groupid=".$groupid;
				}

				$with_node = " and ".DBin_node('h.hostid',get_current_nodeid(!$only_current_node));
//SDI('C: '.$a_groupid.' : '.$a_hostid);

				if($db_host = DBfetch(DBselect("select distinct h.hostid,h.host from hosts h,hosts_groups hg ".$item_table.$group_table.
							" where hg.hostid=h.hostid and hg.groupid in (".$accessed_groups.") "
					.$with_host_status.$with_items.$witth_group.$with_node.
					" order by h.host")))
				{
					$first_hostid_in_group = $db_host["hostid"];
				}

				if($first_hostid_in_group == 0)	$hostid = 0; /* no hosts in selected grpore */

				if($hostid > 0)
				{
					if(!DBfetch(DBselect("select distinct h.hostid from hosts h,hosts_groups hg".$item_table.
						" where hg.hostid=h.hostid and h.hostid=".$hostid.$with_host_status.$with_items.$with_node.
						" and hg.groupid in (".$accessed_groups.") ")))
					{
							$hostid = 0;
					}
				}
				if(($hostid < 0) || ($hostid == 0 && $always_select_first_host == 1)) /* incorrect host */
				{
					$hostid = $first_hostid_in_group;
				}
			}
		}
		
		$group_correct	= ($groupid == $a_groupid) ? 1 : 0;
		$host_correct	= ($hostid == $a_hostid) ? 1 : 0;
		return array(
			"groupid"	=> $groupid,
			"group_correct"	=> $group_correct,
			"hostid"	=> $hostid,
			"host_correct"	=> $host_correct,
			"correct"	=> ($group_correct && $host_correct) ? 1 : 0
			);
	}

	/*
	 * Function: validate_group_with_host
	 *
	 * Description:
	 *     Check available groups and host by user permission
	 *     and check current group an host relations
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 */
	function	validate_group_with_host($perm, $options = array(),$group_var=null,$host_var=null)
	{
		if(is_null($group_var)) $group_var = "web.latest.groupid";
		if(is_null($host_var))	$host_var = "web.latest.hostid";

		$_REQUEST["groupid"]    = get_request("groupid", -1 );
		$_REQUEST["hostid"]     = get_request("hostid", get_profile($host_var,0));

		if($_REQUEST["groupid"] == -1)
		{
			$_REQUEST["groupid"] = get_profile($group_var,0);
			
			if(!in_node($_REQUEST["groupid"])) $_REQUEST["groupid"] = 0;

			if ($_REQUEST["hostid"] > 0 && !DBfetch(DBselect('select groupid from hosts_groups '.
				' where hostid='.$_REQUEST["hostid"].' and groupid='.$_REQUEST["groupid"])))
			{
					$_REQUEST["groupid"] = 0;
			}
		}

		if(in_array("always_select_first_host",$options) && $_REQUEST["hostid"] == 0 && $_REQUEST["groupid"] != 0)
			$_REQUEST["hostid"] = -1;

		$result = get_correct_group_and_host($_REQUEST["groupid"],$_REQUEST["hostid"], $perm, $options);

		$_REQUEST["groupid"]    = $result["groupid"];
		$_REQUEST["hostid"]     = $result["hostid"];
		
		update_profile($host_var,$_REQUEST["hostid"]);
		update_profile($group_var,$_REQUEST["groupid"]);
	}

	/*
	 * Function: validate_group
	 *
	 * Description:
	 *     Check available groups by user permisions
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments:
	 *
	 */
	function	validate_group($perm, $options = array(),$group_var=null)
	{
		if(is_null($group_var)) $group_var = "web.latest.groupid";
		$_REQUEST["groupid"]    = get_request("groupid",get_profile($group_var,0));

		if(!in_node($_REQUEST["groupid"])) $_REQUEST["groupid"] = 0;
		
		$result = get_correct_group_and_host($_REQUEST["groupid"],null,$perm,$options);
		$_REQUEST["groupid"]    = $result["groupid"];

		update_profile($group_var,$_REQUEST["groupid"]);
	}

/* APPLICATIONS */

	/*
	 * Function: db_save_application
	 *
	 * Description:
	 *     Add or update application
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *       If applicationid is NULL add application, in other cases update
	 */
	function	db_save_application($name,$hostid,$applicationid=null,$templateid=0)
	{
		if(!is_string($name)){
			error("incorrect parameters for 'db_save_application'");
			return false;
		}
	
		if($applicationid==null)
			$result = DBselect("select * from applications where name=".zbx_dbstr($name)." and hostid=".$hostid);
		else
			$result = DBselect("select * from applications where name=".zbx_dbstr($name)." and hostid=".$hostid.
				" and applicationid<>$applicationid");

		$db_app = DBfetch($result);
		if($db_app && $templateid==0)
		{
			error("Application '$name' already exists");
			return false;
		}
		if($db_app && $applicationid!=null)
		{ // delete old application with same name
			delete_application($db_app["applicationid"]);
		}

		if($db_app && $applicationid==null)
		{ // if found application with same name update them, adding not needed
			$applicationid = $db_app["applicationid"];
		}

		$host = get_host_by_hostid($hostid);
		
		if($applicationid==null)
		{
			$applicationid_new = get_dbid("applications","applicationid");
			if($result = DBexecute("insert into applications (applicationid,name,hostid,templateid)".
				" values ($applicationid_new,".zbx_dbstr($name).",$hostid,$templateid)"))
					info("Added new application ".$host["host"].":$name");
		}
		else
		{
			$old_app = get_application_by_applicationid($applicationid);
			if($result = DBexecute("update applications set name=".zbx_dbstr($name).",hostid=$hostid,templateid=$templateid".
                                " where applicationid=$applicationid"))
					info("Updated application ".$host["host"].":".$old_app["name"]);
		}

		if(!$result)	return $result;

		if($applicationid==null)
		{// create application for childs
			$applicationid = $applicationid_new;

			$db_childs = get_hosts_by_templateid($hostid);
			while($db_child = DBfetch($db_childs))
			{// recursion
				$result = add_application($name,$db_child["hostid"],$applicationid);
				if(!$result) break;
			}
		}
		else
		{
			$db_applications = get_applications_by_templateid($applicationid);
			while($db_app = DBfetch($db_applications))
			{// recursion
				$result = update_application($db_app["applicationid"],$name,$db_app["hostid"],$applicationid);
				if(!$result) break;
			}
		}

		if($result)
			return $applicationid;

		if($templateid == 0){
			delete_application($applicationid);
		}
		return false;

	}

	/*
	 * Function: add_application
	 *
	 * Description:
	 *     Add application
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 */
	function	add_application($name,$hostid,$templateid=0)
	{
		return db_save_application($name,$hostid,null,$templateid);
	}

	/*
	 * Function: update_application
	 *
	 * Description:
	 *     Update application
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 */
	function	update_application($applicationid,$name,$hostid,$templateid=0)
	{
		return db_save_application($name,$hostid,$applicationid,$templateid);
	}
	
	/*
	 * Function: delete_application
	 *
	 * Description:
	 *     Delete application with all linkages
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 */
	function	delete_application($applicationid)
	{
		$app = get_application_by_applicationid($applicationid);
		$host = get_host_by_hostid($app["hostid"]);

		// first delete child applications
		$db_applications = DBselect("select applicationid from applications where templateid=$applicationid");
		while($db_app = DBfetch($db_applications))
		{// recursion
			$result = delete_application($db_app["applicationid"]);
			if(!$result)	return	$result;
		}

		if($info = DBfetch(DBselect('select name from httptest where applicationid='.$applicationid)))
		{
			info("Application '".$host["host"].":".$app["name"]."' used by scenario '".$info['name']."'");
			return false;
		}
 
		if($info = DBfetch(DBselect('select i.key_, i.description from items_applications ia, items i '.
			' where i.type='.ITEM_TYPE_HTTPTEST.' and i.itemid=ia.itemid and ia.applicationid='.$applicationid)))
		{
			info("Application '".$host["host"].":".$app["name"]."' used by item '".
				item_description($info['description'], $info['key_'])."'");
			return false;
		}

		$result = DBexecute("delete from items_applications where applicationid=$applicationid");

// 		$result = DBexecute("delete from applications where applicationid=$applicationid");
		if($result)
		{
			info("Application '".$host["host"].":".$app["name"]."' deleted");
		}
		return $result;
	}

	function	get_application_by_applicationid($applicationid,$no_error_message=0)
	{
		$result = DBselect("select * from applications where applicationid=".$applicationid);
		$row=DBfetch($result);
		if($row)
		{
			return $row;
		}
		if($no_error_message == 0)
			error("No application with id=[$applicationid]");
		return	false;
		
	}

	function	&get_applications_by_templateid($applicationid)
	{
		return DBselect("select * from applications where templateid=".$applicationid);
	}

	function	get_realhost_by_applicationid($applicationid)
	{
		$application = get_application_by_applicationid($applicationid);
		if($application["templateid"] > 0)
			return get_realhost_by_applicationid($application["templateid"]);

		return get_host_by_applicationid($applicationid);
	}

	function	get_host_by_applicationid($applicationid)
	{
		$sql="select h.* from hosts h, applications a where a.hostid=h.hostid and a.applicationid=$applicationid";
		$result=DBselect($sql);
		$row=DBfetch($result);
		if($row)
		{
			return $row;
		}
		error("No host with applicationid=[$applicationid]");
		return	false;
	}

	function	&get_items_by_applicationid($applicationid)
	{
		return DBselect("select i.* from items i,items_applications ia where i.itemid=ia.itemid and ia.applicationid=$applicationid");
	}

	function	&get_applications_by_hostid($hostid)
	{
		return DBselect('select * from applications where hostid='.$hostid);
	}

	/*
	 * Function: delete_template_applications
	 *
	 * Description:
	 *     Delete applicatios from host by templates
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 *           $templateid can be numeric or numeric array
	 *
	 */
	function        delete_template_applications($hostid, $templateid = null, $unlink_mode = false)
	{
		$db_apps = get_applications_by_hostid($hostid);
		while($db_app = DBfetch($db_apps))
		{
			if($db_app["templateid"] == 0)
				continue;

			if($templateid != null)
			{
				if( !is_array($templateid))
					$templateid = array($templateid);

				unset($skip);
				if( ($tmp_app_data = get_application_by_applicationid($db_app["templateid"])) )
				{
					if( !in_array($tmp_app_data["hostid"], $templateid) )
					{
						$skip = true;
						break;
					}
				}
				if(isset($skip)) continue;
				
			}
			
			if($unlink_mode)
			{
				if(DBexecute("update applications set templateid=0 where applicationid=".$db_app["applicationid"]))
				{
					info("Application '".$db_app["name"]."' unlinked");
				}
			}
			else
			{
				delete_application($db_app["applicationid"]);
			}
		}
	}

	/*
	 * Function: copy_template_applications
	 *
	 * Description:
	 *     Copy applicatios from templates to host
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: !!! Don't forget sync code with C !!!
	 *
	 *           $templateid can be numeric or numeric array
	 *
	 */
	function	copy_template_applications($hostid, $templateid = null, $copy_mode = false)
	{
		if(null == $templateid)
		{
			$templateid = array_keys(get_templates_by_hostid($hostid));
		}
		
		if(is_array($templateid))
		{
			foreach($templateid as $id)
				copy_template_applications($hostid, $id, $copy_mode); // attention recursion
			return;
		}

		$db_tmp_applications = get_applications_by_hostid($templateid);

		while($db_tmp_app = DBfetch($db_tmp_applications))
		{
			add_application(
				$db_tmp_app["name"],
				$hostid,
				$copy_mode ? 0 : $db_tmp_app["applicationid"]);
		}
	}

	/*
	 * Function: validate_templates
	 *
	 * Description:
	 *     Check collisions between templates
	 *
	 * Author:
	 *     Eugene Grigorjev (eugene.grigorjev@zabbix.com)
	 *
	 * Comments: 
	 *           $templateid_list can be numeric or numeric array
	 *
	 */
	function	validate_templates($templateid_list)
	{
		if(is_numeric($templateid_list))return true;
		if(!is_array($templateid_list))	return false;
		if(count($templateid_list)<2)	return true;
		
		$result = true;
		$db_cnt = DBfetch(DBselect('select key_,type,count(*) as cnt from items '.
			' where hostid in ('.implode(',',$templateid_list).') '.
			' group by key_,type order by cnt desc'
			));

		$result &= $db_cnt['cnt'] > 1 ? false : true;

// archaic check. We can have applications with the same name come from several templates
// 		$db_cnt = DBfetch(DBselect('select name,count(*) as cnt from applications '.
// 			' where hostid in ('.implode(',',$templateid_list).') '.
// 			' group by name order by cnt desc'
// 			));

// 		$result &= $db_cnt['cnt'] > 1 ? false : true;

		return $result;
	}

	function	host_js_menu($hostid, $link_text = S_SELECT)
	{
		$add_to = array();
		$delete_from = array();

		$popup_menu_actions = array(
			array(S_SHOW, null, null, array('outer'=> array('pum_oheader'), 'inner'=>array('pum_iheader'))),
			array(S_ITEMS, 'items.php?hostid='.$hostid, array('tw'=>'_blank')),
			array(S_TRIGGERS, 'triggers.php?hostid='.$hostid, array('tw'=>'_blank')),
			array(S_GRAPHS, 'graphs.php?hostid='.$hostid, array('tw'=>'_blank')),
			);

		$db_groups = DBselect('select g.groupid, g.name from groups g left join hosts_groups hg '.
			' on g.groupid=hg.groupid and hg.hostid='.$hostid.
			' where hostid is NULL order by g.name,g.groupid');
		
		while($group_data = DBfetch($db_groups))
		{
			$add_to[] = array($group_data['name'], '?'.
				url_param($group_data['groupid'], false, 'add_to_group').
				url_param($hostid, false, 'hostid')
				);
		}

		$db_groups = DBselect('select g.groupid, g.name from groups g, hosts_groups hg '.
			' where g.groupid=hg.groupid and hg.hostid='.$hostid.
			' order by g.name,g.groupid');
		
		while($group_data = DBfetch($db_groups))
		{
			$delete_from[] = array($group_data['name'], '?'.
				url_param($group_data['groupid'], false, 'delete_from_group').
				url_param($hostid, false, 'hostid')
				);
		}

		if(count($add_to) > 0 || count($delete_from) > 0)
		{
			$popup_menu_actions[] = array(S_GROUPS, null, null,
			array('outer'=> array('pum_oheader'), 'inner'=>array('pum_iheader')));
		}
		if(count($add_to) > 0)
		{
			$popup_menu_actions[] = array_merge(array(S_ADD_TO_GROUP, null, null, 
			array('outer' => 'pum_o_submenu', 'inner'=>array('pum_i_submenu'))), $add_to);
		}
		if(count($delete_from) > 0)
		{
			$popup_menu_actions[] = array_merge(array(S_DELETE_FROM_GROUP, null, null, 
			array('outer' => 'pum_o_submenu', 'inner'=>array('pum_i_submenu'))), $delete_from);
		}

		$mnuActions = new CPUMenu($popup_menu_actions);

		$show = new CLink($link_text, '#', 'action', $mnuActions->GetOnActionJS());

		return $show;
	}
?>

<?php
// check that HFS module is loaded
function zbx_hfs_available ()
{
	return function_exists ("zabbix_hfs_host_availability");
}
?>

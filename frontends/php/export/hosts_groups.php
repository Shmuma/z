<?php
header ("Content-Type: application/json");

function error ($msg)
{
	print "Error: $msg";
	exit;
}

// connect to database
$conn = oci_pconnect ("zabbix", "zabbix", "zabbix.yandex.net");

if (!$conn)
	error ("Failed to connect to database");

// select all sites
$st = oci_parse ($conn, "select groupid, name from groups order by name");

if (!$st)
	error ("Query failed");

oci_execute ($st, OCI_DEFAULT);

$res = array ();
while ($group = oci_fetch_array ($st, OCI_RETURN_NULLS)) {
	$id = (integer)$group[0];
	$name = $group[1];
	$res[$id] = $name;
}

oci_close ($conn);

print json_encode ($res);

?>


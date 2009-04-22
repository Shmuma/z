<?php
	header ("Content-Type: text/plain");

function error ($msg)
{
	print "Error: $msg";
	exit;
}


function parse_memcache_servers ($line)
{
	// ETO:monitor-eto.yandex.net,Fian:monitor-fian.yandex.net,UGR:monitor-ugr.yandex.net,DC3:monitor-dc3.yandex.net,IVA1:monitor-iva1.yandex.net 
	$res = array ();
	foreach (split (",", $line) as $site) {
		$arr = split (":", $site);
		if (count ($arr) == 2)
			$res[$arr[0]] = $arr[1];
	}

	return $res;
}


// parse config line
$mem_srvs = parse_memcache_servers (ini_get ("zabbix.sites_memcache"));

// connect to database
$conn = oci_pconnect ("zabbix", "zabbix", "zabbix.yandex.net");

if (!$conn)
	error ("Failed to connect to database");

// select all sites
$st = oci_parse ($conn, "select siteid, name from sites order by siteid");

if (!$st)
	error ("Sites select failed");

oci_execute ($st, OCI_DEFAULT);

while ($site = oci_fetch_array ($st, OCI_RETURN_NULLS)) {
	if (!array_key_exists ($site[1], $mem_srvs))
		continue;

	// select hosts from this site
	$h_st = oci_parse ($conn, "select host from hosts where siteid=$site[0] and status=0 order by host");

	if (!$h_st)
		continue;

	oci_execute ($h_st, OCI_DEFAULT);

	// make memcache connection to this 
	$memcache = new Memcache;
	$memcache->addServer ($mem_srvs[$site[1]], 11211);
	
	$keys = array ();

	while ($host = oci_fetch_array ($h_st, OCI_RETURN_NULLS))
		$keys[] = "p|$host[0]";

	$res = $memcache->get ($keys);

	foreach ($res as $key => $val) {
		$hosts = split ("\|", $key);
		$host = $hosts[1];
		$strs = unpack ("Its/Iasd/a*sn/", $val);
		if (strlen ($strs["sn"])) {
			$serials = split (": ", $strs["sn"]);
			$serial = trim ($serials[1]);
			print "$host,$serial,$strs[ts]\n";
		}
	}

	$memcache->close ();
}

oci_close ($conn);

?>

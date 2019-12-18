<?php
/* Copyright 2019 Justin Taylor */

   /*This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Justin Taylor <taylor.justin199@gmail.com> */

$configfiledirectory = "../controller-confs/";
$configfilepath = $configfiledirectory . "config.ini";
$config = parse_ini_file($configfilepath, true, INI_SCANNER_RAW); 
$connection = mysqli_connect($config['dbserveraddress'],$config['dbusername'],$config['dbpassword'],$config['db']);
if (!$connection){
  die( "Connection to  MySQL failed");

}

$file_queue = $config['agentfilequeuefolder'];
$agent_action_logs = $config['agentactionlogsfolder'];
$webredirectlocation = $config['webalias'];

$all_hosts_query = mysqli_query($connection, "SELECT Hostname, ClientEnabled, ClientOK, ClientHB, ClientRegDate FROM clients");

$all_groups_query = mysqli_query($connection, "SELECT groupassignments.GroupName, GroupEnabled, COUNT(clientassignments.GroupName) as numofagents FROM groupassignments LEFT JOIN clientassignments ON groupassignments.GroupName = clientassignments.GroupName GROUP BY groupassignments.GroupName, groupassignments.GroupEnabled");

$enabled_hosts_query_with_checked_ip = mysqli_query($connection, "SELECT  clients.Hostname, ClientOK, ClientHB, INET_NTOA(clientinfo.ClientIP) FROM clients INNER JOIN clientinfo ON clients.Hostname = clientinfo.Hostname WHERE ClientEnabled=1");

$all_enabled_hosts_query = mysqli_query($connection, "SELECT * from clients where ClientEnabled=1 and ClientOK=1");

$all_clients_not_ok = mysqli_query($connection, "SELECT * from clients WHERE ClientEnabled=1 AND ClientOK=0");

$num_of_registered_clients = mysqli_num_rows($all_hosts_query);
$num_of_enabled_clients = mysqli_num_rows($all_enabled_hosts_query);
$num_of_clients_not_ok = mysqli_num_rows($all_clients_not_ok);

const kB = 1024;
const MB = 1048576;
const GB = 1073741824;


?>
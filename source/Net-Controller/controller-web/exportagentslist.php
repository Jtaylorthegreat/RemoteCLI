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
session_start(); 
if(!isset($_SESSION['UserData']['Username'])){
  header("location:login.php");
  exit;
  }
require 'defs.php';

$all_agents_csv = mysqli_query($connection, "SELECT clients.Hostname, clients.ClientOK, clients.ClientEnabled, clients.ClientHB, clients.ClientRegDate, clientinitstat.LatestFileAttempt, clientinitstat.LatestFileTime, clientinitstat.LatestActionAttempt, clientinitstat.LatestActionTime, INET_NTOA(clientinfo.ClientIP), clientinfo.OSVER, clientassignments.GroupName, clientassignments.ClientWaiting, clientassignments.ClientFile, clientassignments.ClientFileDependsT, clientassignments.ClientFileTime, clientassignments.ClientWaitingAction, clientassignments.ClientAction, clientassignments.ClientActionDependsT, clientassignments.ClientActionTime from clients join clientinfo on clients.Hostname = clientinfo.Hostname join clientassignments on clientinfo.Hostname = clientassignments.Hostname join clientinitstat on clientassignments.Hostname = clientinitstat.Hostname ORDER BY clients.Hostname ASC");

	if($num_of_registered_clients > 0){
        $outputfilename = "agentlist_" . date('Y-m-d') . ".csv";
        $delimiter = ",";
        $tempoutput = fopen('php://output', 'w');
        $fields = array('Hostname', 'Status', 'Enabled', 'Last check-in', 'Registration date', 'Latest Script Transfer Attempt', 'Latest Script Transfer Attempt Time', 'Latest Command Attempt', 'Latest Command Attempt Time', 'Reported IP', 'OSVER', 'Group', 'Script Transfer at next check-in', 'Script File', 'Script Transfer Waiting on Scheduled Time', 'Script Tranfer Scheduled Time', 'Agent Command at next check-in', 'Command', 'Command Waiting on Scheduled Time', 'Command Scheduled Time');
        fputcsv($tempoutput, $fields, $delimiter);
        while($csvrow = mysqli_fetch_array($all_agents_csv)){
            //Format for cleaner csv output:
            $AEnabled = ($csvrow['ClientEnabled'] == '1')?'Enabled':'Disabled';
            $AOK = ($csvrow['ClientOK'] == '1')?'Online':'Offline';
            if ($csvrow['OSVER'] == 'OS001'){$OS = 'Ubuntu 18.04';}
            if ($csvrow['OSVER'] == 'OS002'){$OS = 'Centos 7';}
            if ($csvrow['OSVER'] == 'OS003'){$OS = 'Centos 8';}
            if ($csvrow['OSVER'] == 'OS004'){$OS = 'Windows';}

            $default_date = "1999-09-09 01:00:00";
            $file_attempt_time = $csvrow['LatestFileTime'];
            $file_schedule_time = $csvrow['ClientFileTime'];
            $action_attempt_time = $scvrow['LatestActionTime'];
            $action_schedule_time = $csvrow['ClientActionTime'];
            if ($file_attempt_time > $default_date){
                if ($csvrow['LatestFileAttempt'] ==1){
                    $LFA = "Successful";
                    $LFT = $csvrow['LatestFileTime'];
                }
                else {
                    $LFA = "Un-Successful";
                    $LFT = $csvrow['LatestFileTime'];
                }
            }
            else {
                $LFA = "N/A";
                $LFT = "N/A";
            }
            if ($action_attempt_time > $default_date){
                if ($csvrow['LatestActionAttempt'] ==1){
                    $LAA = "Successful";
                    $LAT = $csvrow['LatestActionTime'];
                }
                else {
                    $LAA = "Un-Successful";
                    $LAT = $csvrow['LatestActionTime'];
                }
            }
            else {
                $LAA = "N/A";
                $LAT = "N/A";
            }

            if ($file_schedule_time > $default_date){
                $FST = $csvrow['ClientFileTime'];
            }
            else {
                $FST = "N/A";
            }

            if ($action_schedule_time > $default_date){
                $AST = $csvrow['ClientActionTime'];
            }
            else {
                $AST = "N/A";
            }

            if ($csvrow['GroupName'] == "NULL" || $csvrow['GroupName'] == NULL){
                $agentgroup = "N/A";
            }
            else {
                $agentgroup = $csvrow['GroupName'];
            }

            $file_next_check_in = ($csvrow['ClientWaiting'] == '1')?'Yes':'No';
            $action_next_check_in = ($csvrow['ClientWaitingAction'] == '1')?'Yes':'No';

            if ($csvrow['ClientFile'] == "NULL" || $csvrow['ClientFile'] == NULL){
                $agentfile = "No File Assigned";
            }
            else {
                $agentfile = $csvrow['ClientFile'];
            }
            $file_depends_on_schedule = ($csvrow['ClientFileDependsT'] == '1')?'Yes':'No';

            if ($csvrow['ClientAction'] == "NULL" || $csvrow['ClientAction'] == NULL){
                $agentaction = "No Action Assigned";
            }
            else {
                $agentaction = $csvrow['ClientAction'];
            }
            $action_depends_on_schedule = ($csvrow['ClientFileDependsT'] == '1')?'Yes':'No';

            //-----End of csv format values----

            $agentdata = array($csvrow['Hostname'], $AOK, $AEnabled, $csvrow['ClientHB'], $csvrow['ClientRegDate'], $LFA, $LFT, $LAA, $LAT, $csvrow['INET_NTOA(clientinfo.ClientIP)'], $OS, $agentgroup, $file_next_check_in, $agentfile, $file_depends_on_schedule, $FST, $action_next_check_in, $agentaction, $action_depends_on_schedule, $AST);
            fputcsv($tempoutput, $agentdata, $delimiter);
        }
        fseek($tempoutput, 0);
        header('Content-Type: text/csv');
        header('Content-Disposition: attachement; filename="' . $outputfilename . '";');
        fpassthru($tempoutput);

    }
    exit;

?>

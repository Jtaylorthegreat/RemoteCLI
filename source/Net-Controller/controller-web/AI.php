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
$HVALUE=$_REQUEST['rowId'];
$BUILDQUERY = "SELECT clients.Hostname, clients.ClientOK, clients.ClientEnabled, clients.ClientHB, clients.ClientRegDate, clientinitstat.LatestFileAttempt, clientinitstat.LatestFileTime, clientinitstat.LatestActionAttempt, clientinitstat.LatestActionTime, INET_NTOA(clientinfo.ClientIP), clientinfo.OSVER, clientassignments.GroupName, clientassignments.ClientWaiting, clientassignments.ClientFile, clientassignments.ClientFileDependsT, clientassignments.ClientFileTime, clientassignments.ClientWaitingAction, clientassignments.ClientAction, clientassignments.ClientActionDependsT, clientassignments.ClientActionTime from clients join clientinfo on clients.Hostname = clientinfo.Hostname join clientassignments on clientinfo.Hostname = clientassignments.Hostname join clientinitstat on clientassignments.Hostname = clientinitstat.Hostname where clients.Hostname='$HVALUE'";
$query_agent_info = mysqli_query($connection, $BUILDQUERY);
$file_queue = $config['agentfilequeuefolder'];
$script_folder = $file_queue;
echo "<link rel='stylesheet' type='text/css' href='stylesheet.css'/>";

echo "<head><meta http-equiv='content-type' content='text/html; charset=UTF-8'><link rel='shortcut icon' type='image/x-icon' href='favicon.ico' /><title>Net-Controller</title></head>";
echo "<body><header id='header'> RemoteCLI Net-Controller </header>";
echo  "
    <div class='wrapper'>
      		<div class='tabs'>";

      	 //Tab 1:
        echo "<div class='tab'> <input name='css-tabs' id='tab-1'  class='tab-switch' type='radio'> <label for='tab-1' class='tab-label'><a href='overview.php'>Overview</label></a></div>";
        
        
        //Tab 2:
        echo "<div class='tab'> <input name='css-tabs' id='tab-2' checked='checked' class='tab-switch' type='radio'> <label for='tab-2' class='tab-label'><a href='agents.php'>Agents</a></label><div class='tab-content'>";
            

            while($row = mysqli_fetch_array($query_agent_info))
            {
                echo "<b><p>" . $row['Hostname'] . "</p></b>";
                echo "<hr>";
                $file_queue = $config['agentfilequeuefolder'];
                $script_files = scandir($file_queue);
                if ($row['ClientEnabled'] == 1){
                    echo "<b> Agent: </b> Enabled  <br>";
                }
                else {
                    echo "<b> Agent: </b> Disabled <br>";
                }
                if ($row['ClientOK'] == 1){
                    echo "<b>Agent Status: </b><font color='green'> Online </font><br>";
                }
                else {
                    echo "<b>Agent Status: </b><font color='red'> Offline </font><br>";
                }
                echo "<b>Last Heartbeat: </b>" . $row['ClientHB'] . "<br>";
                echo "<b> Client Registered: </b>" . $row['ClientRegDate'] . "<br>";
                echo "<b> Reported IP: </b>" . $row['INET_NTOA(clientinfo.ClientIP)'] . "<br>";
                if ($row['OSVER'] == "OS001"){
                    echo "<b> Reported OS: </b> Ubuntu 18.04 <br>";
                }
                if ($row['OSVER'] == "OS002"){
                    echo "<b> Reported OS: </b> Centos 7 <br>";
                }
                if ($row['OSVER'] == "OS003"){
                    echo "<b> Reported OS: </b> Centos 8 <br>";
                }
                if ($row['OSVER'] == "OS004"){
                    echo "<b> Reported OS: </b> Windows <br>";
                }

                if ($row['GroupName'] == "NULL" || $row['GroupName'] == NULL){
                    echo "<b>Group: </b> Agent not subscribed to any group <br>";
                    echo "<form action='agentgroup.php?agentId={$HVALUE}' method='post'><button type='submit' class='lynkme'>Click here to subscribe agent to group </button></form><br>";
                }
                else {
                    $assignedgroup = $row['GroupName'];
                    echo "<b>Group: </b>" . $row['GroupName'] . "<br>";
                    echo "<form method='post' name='removefromgroup'><button type='submit' name='unsubscribefromgroup' class='lynkme'>Click here to unsubscribe agent from group </button></form>";
                    if(isset($_POST['unsubscribefromgroup'])) {
                        $build_remove_group_query = "UPDATE clientassignments SET GroupName='NULL' WHERE Hostname='$HVALUE'";
                        if (($unsubribe_agent_update = mysqli_query($connection, $build_remove_group_query)) === TRUE ){
                            echo "Agent unsubscribed from $assignedgroup<br>";
                        } 
                        else {
                            echo "Error removing agent from $assignedgroup<br>";
                        }
                    } 
                }
                if ($row['ClientEnabled'] == 1){
                    echo "<form method='post' ><button type='submit' name='disableagent' class='lynkme'>Click here to Disable agent </button></form>";
                    if(isset($_POST['disableagent'])) {
                        $build_disableagent = "UPDATE clients SET ClientEnabled='0' WHERE Hostname='$HVALUE'";
                        if (($disable_agent_update = mysqli_query($connection, $build_disableagent)) === TRUE ){
                            echo "Agent Disabled<br>";
                        } 
                        else {
                            echo "Error Disabling Agent<br>";
                        }

                    }
                }
                else {
                    echo "<form method='post' ><button type='submit' name='enableagent' class='lynkme'>Click here to Enable agent </button></form>";
                    if(isset($_POST['enableagent'])) {
                        $build_enableagent = "UPDATE clients SET ClientEnabled='1' WHERE Hostname='$HVALUE'";
                        if (($enable_agent_update = mysqli_query($connection, $build_enableagent)) === TRUE ){
                            echo "Agent Enabled<br>";
                        } 
                        else {
                            echo "Error Enabling Agent<br>";
                        }

                        
                    }
                }
                echo "<hr class='agent'>";
                echo "<b>Script Transfer: </b> <br>";
                if ($row['ClientWaiting'] == 1){
                    echo "Agent Script Transfer scheduled for next agent heartbeat: <br>";
                    if ($row['ClientFile'] == NULL){
                        echo "<font color='red'>No File Assigned</font><br>";
                    }
                    else {
                        echo "" . $row['ClientFile'] . "<br>";
                    }
                }   
                else {
                    if ($row['ClientFileDependsT'] == 1){
                        echo "Agent Script Transfer scheduled at " .  $row['ClientFileTime'] . ":<br>";
                            if ($row['ClientFile'] == NULL){
                                echo "<font color='red'>No File Assigned</font><br>";
                            }
                            else {
                                echo "" . $row['ClientFile'] . "<br>";
                            }
                    }
                    else {
                        echo " No script transfer pending for agent <br>";
                    }
                }

                $default_date = "1999-09-09 01:00:00";

                $file_attempt_time = $row['LatestFileTime'];
                
                $action_attempt_time = $row['LatestActionTime'];
                if ($file_attempt_time > $default_date){
                    if ($row['LatestFileAttempt'] ==1){
                        echo "<b>Latest script transfer Initialization: </b><br>";
                        echo "<font color='green'> Successful </font> " . $row['LatestFileTime'] . "<br>";
                    }
                    else {
                        echo "<b>Latest script transfer Initialization: </b><br>";
                        echo "<font color='red'> Failed </font> " . $row['LatestFileTime'] . "<br>";
                    }
                }
                echo "<br>";
                echo "<b> Transfer Script:</b><br>";
                //----start of select script drop down----
                echo "<form method ='post'><select name ='selectedscript'> <br>";
                foreach($script_files as $file) {
                    if (!in_array($file,array(".",".."))){
                        echo "<option value = '$file' >" . $file . "</option>";
                    }
                }
                echo "</select>";
                echo "<input type ='submit' name = 'submit' value= 'Transfer Script'><br>";
                echo "<input type ='checkbox' name = 'transferscriptatdate'> at selected time: &nbsp ";
                echo "<input type ='checkbox' name = 'transferscriptnextcheckin'> at next agent check-in <br>";
                echo "<input type ='date' name = 'transferdate' min = '2019-01-01'> &nbsp";
                echo "<input type ='time' name = 'transfertime'><br>"; 
                echo "</form>";
                if(isset($_POST['submit'])) { 
                    if(!isset($_POST['transferscriptnextcheckin']) && !isset($_POST['transferscriptatdate'])){
                        echo "No time selected! Please select a time option <br>";
                    }
                    if(isset($_POST['transferscriptnextcheckin']) && isset($_POST['transferscriptatdate'])){
                        echo "Invalid time submitted, please select 1 time option only <br>";
                    }
                    $pickedscript = $_POST['selectedscript'];
                    $sendscript = $file_queue . $pickedscript;
                    if(isset($_POST['transferscriptnextcheckin']) && !isset($_POST['transferscriptatdate'])){
                        $build_ftransfer_query = "UPDATE clientassignments set ClientWaiting=1, ClientFileDependsT=0, ClientFile='$sendscript' where Hostname='$HVALUE'";
                        if (($mark_agent_fortransfer = mysqli_query($connection, $build_ftransfer_query)) === TRUE ){
                           echo "Agent marked for script transfer upon next check-in";
                        } else {
                            echo "Error marking agent for script transfer";
                        }
                    }
                    if(!isset($_POST['transferscriptnextcheckin']) && isset($_POST['transferscriptatdate'])){
                        if(empty($_POST['transferdate']) || empty($_POST['transfertime'])){
                            echo "Please submit both date and time!<br>";
                        }
                        else {
                            $pickeddate = $_POST['transferdate'];
                            $pickedtime = $_POST['transfertime'];
                            $formattime = $pickeddate . ' ' . $pickedtime . ":00";
                            $build_ftransfertime_query = "UPDATE clientassignments SET ClientFileDependsT=1, ClientWaiting=0, ClientFileTime='$formattime', ClientFile='$sendscript' where Hostname='$HVALUE'";
                            if (($mark_agent_fortransfer_at_time = mysqli_query($connection, $build_ftransfertime_query)) === TRUE ){
                                echo "$pickedscript scheduled for transfer at: $pickedtime on $pickeddate<br>";
                            }
                            else {
                                echo "Error scheduling script transfer for agent<br>";
                            }

                        }

                    }

                }
                //----end of select script drop down----

                echo "<hr class='agent'>";
                echo "<b>Command: </b> <br>";
                if ($row['ClientWaitingAction'] == 1){
                    echo "Agent Command scheduled upon next agent heartbeat: <br>";
                        if ($row['ClientAction'] == NULL){
                            echo "<font color='red'>No Command Assigned</font><br>";
                        }
                        else {
                            echo "" . $row['ClientAction'] . "<br>";
                        }
                }
                else {
                    if ($row['ClientActionDependsT'] == 1){
                        echo "Agent Command scheduled at " .  $row['ClientActionTime'] . ":<br>";
                        if ($row['ClientAction'] == NULL){
                            echo "<font color='red'>No Command Assigned</font><br>";
                        }
                        else {
                            echo "" . $row['ClientAction'] . "<br>";
                        }
                    }
                    else {
                        echo " No action pending for agent  <br>";
                    }
                }
                if ($action_attempt_time > $default_date){
                    if ($row['LatestActionAttempt'] ==1){
                        echo "<b>Latest Command Initialization: </b><br>";
                        echo "<font color='green'> Successful </font> " . $row['LatestActionTime'] . "<br>";
                    }
                    else {
                        echo "<b>Latest Command Initialization: </b><br>";
                        echo "<font color='red'> Failed </font> " . $row['LatestActionTime'] . "<br>";
                    }
                }
                echo "<br>";
                $agent_action_logs = $config['agentactionlogsfolder'];
                $agent_comp_log_dir = $agent_action_logs . $row['Hostname'];
                if(is_dir($agent_comp_log_dir)){
                    $agentlogs = scandir($agent_comp_log_dir);
                    echo "<b>Command Output Logs Available: </b><br>";
                    foreach($agentlogs as $agentlogfile) {
                            if(!in_array($agentlogfile,array(".",".."))) {
                                $open_path = $agent_comp_log_dir . "/" . $agentlogfile;
                                echo "<a href='$open_path'>" . $agentlogfile . "</a><br>";
                                //continue;  
                            }
                    }
                }


                //----start of action text box----
                echo "<br>";
                echo "<b> Run Command: </b><br>";
                echo "<textarea id='actiontext' class='text' cols='50' rows='4' maxlength='250' name='actiontext' form='submitactionform'></textarea>";
                echo "<form method='post' id='submitactionform' name='submitactionform'>";
                echo "<input type ='checkbox' name = 'actionatdate'> at selected time: &nbsp ";
                echo "<input type ='checkbox' name = 'actionnextcheckin'> at next agent check-in <br>";
                echo "<input type ='date' name = 'actiondate' min = '2019-01-01'> &nbsp";
                echo "<input type ='time' name = 'actiontime'>&nbsp"; 
                echo "<input type='reset'><input type='submit' name = 'subaction' value='Run Command'></form>";
                if(isset($_POST['subaction'])) { 
                    if(!isset($_POST['actionnextcheckin']) && !isset($_POST['actionatdate'])){
                        echo "No time selected! Please select a time option <br>";
                    }
                    if(isset($_POST['actionnextcheckin']) && isset($_POST['actionatdate'])){
                        echo "Invalid time submitted, please select 1 time option only <br>";
                    }
                    $submittedtext = $_POST['actiontext'];
                    if(isset($_POST['actionnextcheckin']) && !isset($_POST['actionatdate'])){
                        if(empty($_POST['actiontext'])){
                            echo "no command submitted <br>";
                        }
                        else { 
                            $build_action_update = "UPDATE clientassignments set ClientWaitingAction=1, ClientActionDependsT=0, ClientAction=('$submittedtext') where Hostname='$HVALUE'";
                            if (($mark_agent_action = mysqli_query($connection, $build_action_update)) === TRUE){
                                echo "$HVALUE marked for action upon next check-in <br>";
                            }
                            else {
                                echo "Error marking $HVALUE to run action<br>";
                            }
                        }
                    }
                    if(!isset($_POST['actionnextcheckin']) && isset($_POST['actionatdate'])){
                        if(empty($_POST['actiondate']) || empty($_POST['actiontime'])){
                            echo "Please submit both date and time!<br>";
                        }
                        else {
                            if(empty($_POST['actiontext'])){
                               echo "no command submitted <br>";
                            }
                            else {
                                $pickedadate = $_POST['actiondate'];
                                $pickedatime = $_POST['actiontime'];
                                $formatatime = $pickedadate . ' ' . $pickedatime . ":00";
                                $build_atime_update = "UPDATE clientassignments SET ClientActionDependsT=1, ClientWaitingAction=0, ClientActionTime='$formatatime', ClientAction=('$submittedtext') where Hostname='$HVALUE'";
                                if (($mark_agent_action_at_time = mysqli_query($connection, $build_atime_update)) === TRUE ){
                                    echo "$HVALUE scheduled to run command  at: $pickedatime on $pickedadate<br>";
                                }
                                else {
                                    echo "Error scheduling command for agent<br>";
                                }
                            }
                        }
                    }
                    
                }
                //----end of action text box----
                echo "<hr>";
                echo "<br><br><br><br>";
                echo "<form method='post'>";
                echo "<input type ='checkbox' name = 'checkdeleteA'><font color='red'>Delete Agent </font>&nbsp";
                echo "<input type ='submit' name = 'deleteagent' value = 'Delete'></form>";
                echo "(This only removes the agent from the controller's database, please also be sure to stop, disable and uninstall the agent daemon.)";
                if(isset($_POST['deleteagent'])){
                    if(empty($_POST['checkdeleteA'])){
                        echo "Please check the box if you want to delete this agent";
                    }
                    else {
                        $build_delete_agent = "DELETE FROM clients, clientinitstat, clientinfo, clientassignments USING clients join clientinfo on clients.Hostname = clientinfo.Hostname join clientassignments on clientinfo.Hostname = clientassignments.Hostname join clientinitstat on clientassignments.Hostname = clientinitstat.Hostname where clients.Hostname='$HVALUE'";
                        if(($delete_agent_from_db = mysqli_query($connection, $build_delete_agent)) === TRUE ) {
                            $successAredirect = $webredirectlocation . "agents.php";
                            header("location: $successAredirect");
                        }
                        else {
                            echo "Error deleting $HVALUE !<br>";
                        }
                    }
                }



                mysqli_close($connection);  
            }
            echo "</div>
            </div>";




        //Tab 3:
        echo "<div class='tab'> <input name='css-tabs' id='tab-3' class='tab-switch' type='radio'> <label for='tab-3' class='tab-label'><a href='groups.php'> Agent Groups</a></label></div>";

        //Tab 4:
        echo "<div class='tab'> <input name='css-tabs' id='tab-4' class='tab-switch' type='radio'> <label for='tab-4' class='tab-label'><a href='scriptqueue.php'>Script Queue</a></label></div>";

        //Tab 5:
        echo "<div class='tab'> <input name='css-tabs' id='tab-5' class='tab-switch' type='radio'> <label for='tab-5' class='tab-label'><a href='downloads.php'>Download Agents</a></label></div>";

        //Tab 6:
        echo "<div class='tab'> <input name='css-tabs' id='tab-6' class='tab-switch' type='radio'> <label for='tab-6' class='tab-label'><a href='logout.php'>Logout</a></label></div>";

echo "</div>
<div class='push'></div>
<div class ='footer-container'><footer class='footer'> . </footer></div></div>";


echo "</body>";
?>  

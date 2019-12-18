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
$GVALUE=$_REQUEST['rowId'];
$BUILDGQUERY="SELECT groupassignments.GroupName, GroupEnabled, GroupWaiting, GroupFile, GroupFileDependsT, GroupFileTime, GroupWaitingAction, GroupAction, GroupActionDependsT, GroupActionTime, COUNT(clientassignments.GroupName) as numofagents FROM groupassignments LEFT JOIN clientassignments ON groupassignments.GroupName = clientassignments.GroupName WHERE groupassignments.GroupName='$GVALUE' GROUP BY groupassignments.GroupName, groupassignments.GroupEnabled, groupassignments.GroupWaiting, groupassignments.GroupFile,  groupassignments.GroupFileDependsT, groupassignments.GroupFileTime, groupassignments.GroupWaitingAction, groupassignments.GroupAction, groupassignments.GroupActionDependsT, groupassignments.GroupActionTime";

$query_group_info = mysqli_query($connection, $BUILDGQUERY);
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
        echo "<div class='tab'> <input name='css-tabs' id='tab-2' class='tab-switch' type='radio'> <label for='tab-2' class='tab-label'><a href='agents.php'> Agents</a></label></div>";



        //Tab 3:
        echo "<div class='tab'> <input name='css-tabs' id='tab-3' checked='checked' class='tab-switch' type='radio'> <label for='tab-3' class='tab-label'><a href='groups.php'>Agent Groups</a></label>
          <div class='tab-content'>";
            while($row = mysqli_fetch_array($query_group_info))
            {
                echo "<b><p>" . $row['GroupName'] . "</p></b>";
                echo "<hr>";
                if ($row['GroupEnabled'] == 1){
                    echo "<b> Group: </b> Enabled <br>";
                }
                else {
                    echo "<b> Group: </b> Disabled <br>";
                }
                echo "<b>Agents in group:</b> " . $row['numofagents'] . "<br>";
                $agentcount = $row['numofagents'];
                if ($agentcount > 0){
                    echo "<form action='GA.php?rowId={$GVALUE}' method='post'><button type='submit' class='lynkme'> View Agents subscribed to this group</button></form>";
                     echo "<form action='removeagents.php?rowId={$GVALUE}' method='post'><button type='submit' class='lynkme'> Remove Agents from this group</button></form>";
                }
                    echo "<form action='gassign.php?rowId={$GVALUE}' method='post'><button type='submit' class='lynkme'> Assign Agents to this group</button></form>";

                if ($row['GroupEnabled'] == 1){
                    echo "<form method='post'><button type='submit' name='disablegroup' class='lynkme'>Click here to Disable Group </button></form>";
                    if(isset($_POST['disablegroup'])) {
                        $build_disablegroup = "UPDATE groupassignments SET GroupEnabled='0' WHERE GroupName='$GVALUE'";
                        if (($disable_group_update = mysqli_query($connection, $build_disablegroup)) === TRUE ){
                            echo "Group Disabled<br>";
                        } 
                        else {
                            echo "Error Disabling Group<br>";
                        }

                    }
                }
                else {
                    echo "<form method='post'><button type='submit' name='enablegroup' class='lynkme'>Click here to Enable Group </button></form>";
                    if(isset($_POST['enablegroup'])) {
                        $build_enablegroup = "UPDATE groupassignments SET GroupEnabled='1' WHERE GroupName='$GVALUE'";
                        if (($enable_group_update = mysqli_query($connection, $build_enablegroup)) === TRUE ){
                            echo "Group Enabled<br>";
                        } 
                        else {
                            echo "Error Enabling Group<br>";
                        }

                        
                    }
                }

                echo "<br>";
                echo "<hr class='agent'>";


                echo "<b>Script Transfer: </b> <br>";
                if ($row['GroupWaiting'] == 1){
                    echo "Group Script Transfer scheduled opon next heartbeat for script: <br>";
                    if ($row['GroupFile'] == NULL){
                            echo "<font color='red'>No Script File Assigned</font><br>";
                        }
                        else {
                            echo "" . $row['GroupFile'] . "<br>";
                        }

                }
                else {
                    if ($row['GroupFileDependsT'] == 1){
                        echo "Group Script Transfer scheduled at " .  $row['GroupFileTime'] . ":<br>";
                         if ($row['GroupFile'] == NULL){
                            echo "<font color='red'>No Script File Assigned</font><br>";
                        }
                        else {
                            echo "" . $row['GroupFile'] . "<br>";
                        }
                    }
                    else {
                        echo " No script transfer pending for group <br>";
                    }
                }
                echo "<br>";
                echo "<b>Transfer Script:</b>";
                //----start of select script drop down---- 
                $script_files = scandir($file_queue);

                echo "<br>";
                echo "<form method ='post'><select name ='selectedscript'> <br>";
                foreach($script_files as $file) {
                    if (!in_array($file,array(".",".."))){
                        echo "<option value = '$file' >" . $file . "</option>";
                    }
                }
                echo "</select>";
                echo "<input type ='submit' name = 'transfer' value= 'Transfer Script'><br>";
                echo "<input type ='checkbox' name = 'transferscriptatdate'> at selected time: &nbsp ";
                echo "<input type ='checkbox' name = 'transferscriptnextcheckin'> at next agent check-in <br>";
                echo "<input type ='date' name = 'transferdate' min = '2019-01-01'> &nbsp";
                echo "<input type ='time' name = 'transfertime'><br>";
                echo "</form>";
                if(isset($_POST['transfer'])) { 
                    if(!isset($_POST['transferscriptnextcheckin']) && !isset($_POST['transferscriptatdate'])){
                        echo "No time selected! Please select a time option <br>";
                    }
                    if(isset($_POST['transferscriptnextcheckin']) && isset($_POST['transferscriptatdate'])){
                        echo "Invalid time submitted, please select 1 time option only <br>";
                    }
                    $pickedscript = $_POST['selectedscript'];
                    $sendscript = $file_queue . $pickedscript;
                    if(isset($_POST['transferscriptnextcheckin']) && !isset($_POST['transferscriptatdate'])){
                        $build_Gftransfer_query = "UPDATE groupassignments set GroupWaiting=1, GroupFileDependsT=0, GroupFile='$sendscript' where GroupName='$GVALUE'";
                        if (($mark_group_fortransfer = mysqli_query($connection, $build_Gftransfer_query)) === TRUE ){
                           echo "$GVALUE marked for script transfer upon next agent check-in";
                        } else {
                            echo "Error marking $GVALUE for script transfer";
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
                            $build_Gtransfertime_query = "UPDATE groupassignments SET GroupFileDependsT=1, GroupWaiting=0, GroupFileTime='$formattime', GroupFile='$sendscript' where GroupName='$GVALUE'";
                            if (($mark_group_fortransfer_at_time = mysqli_query($connection, $build_Gtransfertime_query)) === TRUE ){
                                echo "$pickedscript scheduled for transfer at: $pickedtime on $pickeddate<br>";
                            }
                            else {
                                echo "Error scheduling script transfer for $GVALUE<br>";
                            }

                        }

                    }

                }
                
                //----end of select script drop down----

                echo "<hr class='agent'>";
                echo "<b>Command: </b> <br>";
                if ($row['GroupWaitingAction'] == 1){
                    echo "Group Command scheduled upon next agent heartbeat: <br>";
                    if ($row['GroupAction'] == NULL){
                            echo "<font color='red'>No Command Assigned</font><br>";

                        }
                        else {
                            echo "" . $row['GroupAction'] . "<br>";
                        }
                }
                else {
                    if ($row['GroupActionDependsT'] == 1){
                        echo "Group Command scheduled at " .  $row['GroupActionTime'] . ":<br>";
                        if ($row['GroupAction'] == NULL){
                            echo "<font color='red'> No Command Assigned</font><br>";

                        }
                        else {
                            echo "" . $row['GroupAction'] . "<br>";
                        }
                    }
                    else {
                        echo " No command pending for group  <br>";
                    }
                }

                echo "<br>";
                echo "<b>Run Command:</b>";

                
                //----start of action text box----
                echo "<br>";
                echo "<textarea id='actiontext' class='text' cols='50' rows='4' maxlength='250' name='actiontext' form='submitactionform'></textarea>";
                echo "<form method='post' id='submitactionform' name='submitactionform'>";
                echo "<input type ='checkbox' name = 'actionatdate'> at selected time: &nbsp ";
                echo "<input type ='checkbox' name = 'actionnextcheckin'> at next agent check-in <br>";
                echo "<input type ='date' name = 'actiondate' min = '2019-01-01'> &nbsp";
                echo "<input type ='time' name = 'actiontime'> &nbsp"; 
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
                            $build_group_action_update = "UPDATE groupassignments set GroupWaitingAction=1, GroupActionDependsT=0, GroupAction=('$submittedtext') where GroupName='$GVALUE'";
                            if (($mark_group_action = mysqli_query($connection, $build_group_action_update)) === TRUE){
                                echo "$GVALUE marked to run command upon next agent check-in <br>";
                            }
                            else {
                                echo "Error marking $GVALUE to run command<br>";
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
                                $formatgtime = $pickedadate . ' ' . $pickedatime . ":00";
                                $build_gtime_update = "UPDATE groupassignments SET GroupActionDependsT=1, GroupWaitingAction=0, GroupActionTime='$formatgtime', GroupAction=('$submittedtext') where GroupName='$GVALUE'";
                                if (($mark_group_action_at_time = mysqli_query($connection, $build_gtime_update)) === TRUE ){
                                    echo "$GVALUE scheduled to run command  at: $pickedatime on $pickedadate<br>";
                                }
                                else {
                                    echo "Error scheduling command for $GVALUE<br>";
                                }

                            }
                        }
                    }
                }
                //----end of action text box----
                echo "<hr>";
                echo "<br><br><br><br>";
                echo "<form method='post'>";
                echo "<input type ='checkbox' name = 'checkdeleteG'><font color='red'>Delete Group </font>&nbsp";
                echo "<input type ='submit' name = 'deletegroup' value = 'Delete'></form>";
                if(isset($_POST['deletegroup'])){
                    if(empty($_POST['checkdeleteG'])){
                        echo "Please check the box if you want to delete this group";
                    }
                    else {
                        $build_delete_group = "DELETE FROM groupassignments WHERE GroupName='$GVALUE'";
                        if(($delete_group_from_db = mysqli_query($connection, $build_delete_group)) === TRUE ) {
                            $successredirect = $webredirectlocation . "groups.php";
                            header("location: $successredirect");
                        }
                        else {
                            echo "Error deleting $GVALUE !<br>";
                        }
                    }
                }

                mysqli_close($connection);
            }


        echo "</div>
        </div>";  


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
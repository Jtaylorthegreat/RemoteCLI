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
          <div class='tab-content'>
           <table class='a'>
            <tr class='a'><th class='a'>Group</th><th class='a'>Enabled</th><th class='a'>Agents in Group</th></tr>";
            while($row = mysqli_fetch_array($all_groups_query))
            {

            echo "<tr class='a'>";
            echo "<td class='a'><form action='GI.php?rowId={$row['GroupName']}' method='post'><button type='submit' class='lynkme'>" . $row['GroupName'] . "</button></form></td>";
            if ($row['GroupEnabled'] == 1){
                echo "<td class='a'> Enabled </td>";
            }
            else {
                echo "<td class='a'> Disabled </td>";

            }
            echo "<td class='a'>" . $row['numofagents'] . "</td>";
            echo "</tr>";
            }

            echo "</table>"; 
            echo "<br><br>";
            echo "<b> Create Group: </b><br>";
            echo "<form method='post'><input type='text' name='newgroupname'> <input type= 'submit' name='creategroup' value='Create'></form>";
            if(isset($_POST['creategroup'])){
                if(empty($_POST['newgroupname'])){
                    echo "Please enter a group name";
                }
                else {
                    $new_group_name = $_POST['newgroupname'];
                    $build_check_duplicates = "SELECT * FROM groupassignments WHERE GroupName='$new_group_name'";
                    $check_duplicate_query = mysqli_query($connection, $build_check_duplicates);
                    $num_of_GroupNames = mysqli_num_rows($check_duplicate_query);
                    if ($num_of_GroupNames > 0 ){
                        echo "$new_group_name already exists please enter a new group name<br>";
                    }
                    else {
                        $build_create_new_group = "INSERT INTO groupassignments (GroupName, GroupEnabled, GroupWaiting, GroupFile, GroupFileDependsT, GroupFileTime, GroupWaitingAction, GroupAction, GroupActionDependsT, GroupActionTime) VALUES ('$new_group_name', '0', '0', 'NULL', '0', '1999-09-09 01:00:00', '0', 'NULL', '0', '1999-09-09 01:00:00')";
                        if (($insert_new_group = mysqli_query($connection, $build_create_new_group)) === TRUE ){
                            $successredirect = $webredirectlocation . "groups.php";
                            header("location: $successredirect");
                        }
                        else {
                            echo "Error creating new group: $new_group_name<br>";
                        }
                    }
                }
            }

            mysqli_close($connection);


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
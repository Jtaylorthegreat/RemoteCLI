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
$BUILDREMOVEQUERY="SELECT Hostname FROM clientassignments WHERE GroupName='$GVALUE'";
$query_group_agents = mysqli_query($connection, $BUILDREMOVEQUERY);
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
            
            echo "<b> Select Agents to remove from group: </b><br>";
            echo "<hr>";

            while($row = mysqli_fetch_array($query_group_agents))
            {
                $agent_option = $row['Hostname'];
                echo "<form method='post'><input type='checkbox' name='checkedagents[]' value = '$agent_option'> $agent_option <br>";
            }
            echo "<br><input type='submit' name = 'removeagents' value='Remove Agents'></form>";
            if(isset($_POST['removeagents'])){
                if(empty($_POST['checkedagents'])){
                    echo "Please select agents to remove from group<br>";
                }
                else {
                    foreach ($_POST['checkedagents'] as $pickedagents) {
                        $build_remove_agent_group = "UPDATE clientassignments SET GroupName='NULL' WHERE Hostname='$pickedagents'";
                        if (($remove_agents_to_group = mysqli_query($connection, $build_remove_agent_group)) === TRUE ){
                            echo "$pickedagents removed from group<br>";
                        } 
                        else {
                            echo "Error removing $pickedagents from group <br>";
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
        echo "<div class='tab'> <input name='css-tabs' id='tab-6' class='tab-switch' type='radio'> <label for='tab-6' class='tab-label'><a href='Logout.php'>Logout</a></label></div>";



echo "</div>
<div class='push'></div>
<div class ='footer-container'><footer class='footer'> . </footer></div></div>";


echo "</body>";
?>  
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
echo "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

echo "<head><meta http-equiv='content-type' content='text/html; charset=UTF-8'><link rel='shortcut icon' type='image/x-icon' href='favicon.ico' /><title>Net-Controller</title></head>";
echo "<body><header id='header'> RemoteCLI Net-Controller </header>";
echo  "
		<div class='wrapper'>
      		<div class='tabs'>";

      	//Tab 1:
        echo "<div class='tab'> <input name='css-tabs' id='tab-1'  class='tab-switch' type='radio'> <label for='tab-1' class='tab-label'><a href='overview.php'>Overview</label></a></div>";

        //Tab 2:
        echo "<div class='tab'> <input name='css-tabs' id='tab-2' checked='checked' class='tab-switch' type='radio'> <label for='tab-2' class='tab-label'><a href='agents.php'>Agents</a></label><div class='tab-content'>
            <table class='a'>
            <tr class='a'><th class='a'>Hostname</th><th class='a'>Enabled</th><th class='a'>Status</th><th class='a'>Last check-in</th><th class='a'>Registration date</th></tr>";
            while($row = mysqli_fetch_array($all_hosts_query))
            {

            echo "<tr class='a'>";
            echo "<td class='a'><form action='AI.php?rowId={$row['Hostname']}' method='post'><button type='submit' class='lynkme'>"  . $row['Hostname'] . "</button></form></td>";
            if ($row['ClientEnabled'] == 1){
                echo "<td class='a'> Enabled </td>";
            }
            else {
                echo "<td class='a'> Disabled </td>";

            }
            if ($row['ClientOK'] == 1){
                echo "<td class='a'><font color='green'> Online </font></td>";
            }
            else {
                echo "<td class='a'><font color='red'> Offline </font></td>";

            }
            echo "<td class='a'>" . $row['ClientHB'] . "</td>";
            echo "<td class='a'>" . $row['ClientRegDate'] . "</td>";
            }
            mysqli_close($connection);
            
            echo "</tr>";
            echo "</table>"; 
            
            //----start export all agents csv----
            echo "<br>";
            echo "<form action='exportagentslist.php' method='post'><button type='submit' class='lynkme'> Download CSV </button></form>";
            

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
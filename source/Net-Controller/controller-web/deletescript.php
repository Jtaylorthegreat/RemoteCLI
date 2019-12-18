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
        echo "<div class='tab'> <input name='css-tabs' id='tab-3' class='tab-switch' type='radio'> <label for='tab-3' class='tab-label'><a href='groups.php'> Agent Groups</a></label></div>";



		    //Tab 4:
        echo "<div class='tab'> <input name='css-tabs' id='tab-4' checked='checked' class='tab-switch' type='radio'> <label for='tab-4' class='tab-label'><a href='scriptqueue.php'>Script Queue</a></label>
          <div class='tab-content'>";

            $file_queue = $config['agentfilequeuefolder'];
            $script_files = scandir($file_queue);
            $num_of_scripts = count($script_files);
            echo "<form method ='post'><select name = 'pickedscript[]' multiple size = $num_of_scripts>";

            foreach($script_files as $file) {
              if (!in_array($file,array(".","..")))
              {
                $script_size = filesize($file_queue.$file);
                if ( $script_size >= GB){
                  $script_rsize =  round($script_size / GB, 3);
                  echo "<option value = '$file' >" . $file . "</option>";
                  continue;
                }
                if ( $script_size >= MB){
                  $script_rsize =  round($script_size / MB, 3);
                  echo "<option value = '$file' >" . $file . "</option>";
                  continue;
                }
                if ( $script_size >= kB){
                  $script_rsize =  round($script_size / kB, 3);
                  echo "<option value = '$file' >" . $file . "</option>";
                  continue;
                }
                else {
                  echo "<option value = '$file' >" . $file . "</option>";
                  continue;
                }
              }
            }
            echo "</select>";
            echo "<input type = 'submit' name = 'submit' value = Submit>";

            echo "</form>";
            // Check if form is submitted successfully 
            if(isset($_POST["submit"])) { 
              // Check if any option is selected 
            if(isset($_POST["pickedscript"])) { 
              // Retrieving each selected option 
              foreach ($_POST['pickedscript'] as $pickedscript) {  
                $removefile = $file_queue . $pickedscript;
                $delfile = fopen($removefile, 'w') or die("can't open file");
                fclose($delfile);
                if (!unlink($removefile)) {
                  echo ("Error deleting $pickedscript<br>");
                }
                else {
                  echo ("Deleted: $pickedscript <br>");
                }
              }
            } 
            else
              echo "Please select a script file for removal"; 
            } 

          echo "</div>
        </div>";  

        //Tab 5:
        echo "<div class='tab'> <input name='css-tabs' id='tab-5' class='tab-switch' type='radio'> <label for='tab-5' class='tab-label'><a href='downloads.php'>Download Agents</a></label></div>";

        //Tab 6:
        echo "<div class='tab'> <input name='css-tabs' id='tab-6' class='tab-switch' type='radio'> <label for='tab-6' class='tab-label'><a href='logout.php'>Logout</a></label></div>";

echo "</div>
<div class='push'></div>
<div class ='footer-container'><footer class='footer'> . </footer></div></div>";


echo "</body>";
?>  
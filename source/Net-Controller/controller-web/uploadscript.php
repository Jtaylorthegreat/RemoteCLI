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
if(isset($_POST['submit'])) { 
  
    $file_queue = $config['agentfilequeuefolder'];
	$script_folder = $file_queue;
    // Define maxsize for files  
    $maxsize = 300 * MB;  
  
    if(!empty(array_filter($_FILES['files']['name']))) { 
  
        foreach ($_FILES['files']['tmp_name'] as $key => $value) { 
              
            $file_tmpname = $_FILES['files']['tmp_name'][$key]; 
            $file_name = $_FILES['files']['name'][$key]; 
            $file_size = $_FILES['files']['size'][$key]; 
            $file_ext = pathinfo($file_name, PATHINFO_EXTENSION); 
  
            $filepath = $script_folder.$file_name; 
  
                if ($file_size > $maxsize)          
                    echo "Error: File size is larger than the allowed limit.";  
  
                if(file_exists($filepath)) { 
                    $filepath = $script_folder.time().$file_name; 
                      
                    if( move_uploaded_file($file_tmpname, $filepath)) { 
                        echo "{$file_name} successfully uploaded <br />"; 
                    }  
                    else {  
                        echo "Error uploading {$file_name} <br />";  
                    } 
                } 
                else { 
                  
                    if( move_uploaded_file($file_tmpname, $filepath)) { 
                        $successredirect = $webredirectlocation . "scriptqueue.php";
                        header("location: $successredirect");
                    } 
                    else {                      
                        echo "Error uploading {$file_name} <br />";  
                    } 
                } 
        } 
    }  
    else { 
          
        echo "No files selected."; 
    } 
}  
?>

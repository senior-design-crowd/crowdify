<?php
	$userName = $_POST["user"];
	$addr = $_POST["addr"];
	$date = $_POST["date"];
	$dirs = $_POST["dirs"];
	
	exec("restore_now $date $dirs $userName $addr > output_backup.txt");
	echo "DONE";
?>
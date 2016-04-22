<?php
	$userName = $_POST["user"];
	$addr = $_POST["addr"];
	
	exec("backup_now $userName $addr > output_backup.txt");
	echo "DONE";
?>
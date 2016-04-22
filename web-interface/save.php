<?php
	$added_string = $_POST["add"];
	$removed_string = $_POST["remove"];
	$userName = $_POST["user"];
	$addr = $_POST["addr"];
	$added_excludes_string = $_POST["add_ex"];
	$removed_excludes_string = $_POST["remove_ex"];
	$date_and_time = $_POST["dateAndTime"];
	
	exec("mod_dirs $added_string $removed_string $added_excludes_string $removed_excludes_string $date_and_time $userName $addr > output.txt");
	echo "DONE";
?>
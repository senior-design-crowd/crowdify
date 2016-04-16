<?php
	$added_string = $_POST["add"];
	$userName = $_POST["user"];
	$addr = $_POST["addr"];
	
	exec("add_dirs $added_string $userName $addr");
	echo "DONE";
?>
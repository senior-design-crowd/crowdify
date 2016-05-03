<?php
$date = $_POST["date"];
$user = $_POST["user"];
$addr = $_POST["addr"];

exec('getDirsForDate $date $user $addr');

$fp = fopen($date . ".txt", "r");

$data = fgets($fp);

echo $data;
?>
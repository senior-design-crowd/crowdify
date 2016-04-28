<?php
$date = $_POST["date"];

exec("getDirsForDate $date $_POST['user'] $_POST['addr']");

$fp = fopen($date . ".txt", "r");

$data = fgets($fp);

echo $data;
?>
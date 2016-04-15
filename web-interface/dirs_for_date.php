<?php
$date = $_POST["date"];

$fp = fopen($date . ".txt", "r");

$data = fgets($fp);

echo $data;
?>
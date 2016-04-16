<!DOCTYPE HTML>
<html lang="en">
<head>
	<title> Backup Interface </title>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<script src="http://code.jquery.com/jquery-1.9.1.js"></script>
	<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css">
	<link rel="stylesheet" href="css/styles.css">
	<script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/js/bootstrap.min.js"></script>
	<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js"></script>
	<link rel="stylesheet" href="jquery.timepicker.css">
	<script src="jquery.timepicker.js"></script>
	<script>
	//this function is for the directories to backup table
	
	var dirs_for_date;
	var added = new Array();
	var removed = new Array();
	var user = "<?php echo $GET_['user']?>";
	var addr = "<?php echo $_SERVER['REMOTE_ADDR']?>";
	$(function() {

		/* Get all rows from your 'table' but not the first one 
		 * that includes headers. */
		var rows = $('.dirs_to_backup');

		/* Create 'click' event handler for rows */
		rows.on('click', function(e) {

			/* Get current row */
			var row = $(this);

			/* Check if 'Ctrl', 'cmd' or 'Shift' keyboard key was pressed
			 * 'Ctrl' => is represented by 'e.ctrlKey' or 'e.metaKey'
			 * 'Shift' => is represented by 'e.shiftKey' */
			if(row.hasClass('highlight')){
				row.removeClass('highlight');
			}
			else{
			if ((e.ctrlKey || e.metaKey) || e.shiftKey) {
				/* If pressed highlight the other row that was clicked */
				row.addClass('highlight');
			} else {
				/* Otherwise just highlight one row and clean others */
				rows.removeClass('highlight');
				row.addClass('highlight');
			}
			}

		});
	}
	);
		
		
		//this function is for the dates table
		$(function() {

		/* Get all rows from your 'table'*/
		var rows = $('.dates');

		/* Create 'click' event handler for rows */
		rows.on('click', function(e) {

			/* Get current row */
			var row = $(this);

			if(row.hasClass('highlight')){
				row.removeClass('highlight');
			}
			else{
			
				/* Otherwise just highlight one row and clean others */
				rows.removeClass('highlight');
				row.addClass('highlight');
			}
			
			//now use ajax to load the directory information for that date
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function(){
				if(xhttp.readyState == 4 && xhttp.status == 200){
					dirs_for_date = xhttp.responseText;
					set_restore_dirs();
				}
			};
			xhttp.open("POST", "dirs_for_date.php", true);
			xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			var date = row.find('td:eq(0)').html();
			xhttp.send("date=" + date);
			});

		});
	
		function set_restore_dirs(){
			var new_tbody = document.createElement('tbody');
			var dirs = dirs_for_date.split(",");
			for(var i = 0; i < dirs.length; i++){
				var newRow = new_tbody.insertRow(0);
				var newCell = newRow.insertCell(0);
				var newText = document.createTextNode(dirs[i]);
				newCell.appendChild(newText);
			}
			$(new_tbody).attr("id", "dirs_to_restore");
			var old_tbody = document.getElementById('dirs_to_restore');
			old_tbody.parentNode.replaceChild(new_tbody, old_tbody);
		}
		function add(){
			var new_dir = document.getElementById("add_dir").value;
			if (new_dir == "") return;
			added.push(new_dir);
			var tbody = document.getElementById('dirs_to_backup');
			var newRow = tbody.insertRow(0);
			var newCell = newRow.insertCell(0);
			var newText = document.createTextNode(new_dir);
			newCell.appendChild(newText);
			document.getElementById("add_dir").value = "";
		}
		function save(){
			var added_string;
			if(added.length == 0) return;
			else added_string = added[0];
			
			var i;
			for(i = 1; i < added.length;i++){
				added_string += ",";
				added_string += added[i];
			}
			
			console.log(added_string);

			
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function(){
				if(xhttp.readyState == 4 && xhttp.status == 200){
					alert("save successful");
				}
			};
			xhttp.open("POST", "save.php", true);
			xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			xhttp.send("add=" + added_string + "&user=" + user + "&addr=" + addr);
			
		}
		function registerHandlers(){
			//document.getElementById("Add").onclick = add();
			$('#Add').on('click', add);
			$('#save').on('click', save);
		}
	</script>
</head>
<body onload="registerHandlers()">
	<div class = "container-fluid">
		<div class = "page-header">
			<h1> Backup Settings </h1>
			<input id= "save" type="submit" class="btn btn-default" name="Save Settings" value = "Save Settings"></input>
		</div>
		<div class = "row">
			<div class = "col-sm-4">
				<h2>
					Choose Folders
				</h2>
				<div id="t1" class="tableholder">
					<table class="table" id="dirs_to_backup">
						<tbody>
							<?php
								$fp = fopen("files.txt", "r");
								$data = "";
								$data = fgets($fp);
								$dirs = explode(',', $data);
								foreach($dirs as $itemName){
									echo "<tr class='dirs_to_backup'><td>$itemName</td></tr>";
								}
								?>
						</tbody>
					</table>
				</div>
				<input id = "add_dir" class ="text" type="text"/>
				<div class="row folderbuttonrow">
					<input id="Add" type="button" class="btn btn-default" name="Add" value = "Add"></input>
					<input type="button" class="btn btn-default pull-right" name="Remove" value="Remove"></input>
				</div>
				<h2>
					Excludes
				</h2>
				<div id="t1" class="tableholder">
					<table class="table">
						<tbody>
							<?php
							$fp = fopen("excludes.txt", "r");
							$data = "";
							$data = fgets($fp);
							$dirs = explode(',', $data);
							foreach($dirs as $itemName){
								echo "<tr class='excludes'><td>$itemName</td></tr>";
							}
							?>
						</tbody>
					</table>
				</div>
				<input id = "add_ex" class ="text" type="text"/>
				<div class="row folderbuttonrow">
				<input type="submit" class="btn btn-default" name="Add" value="Add"></input>
				<input type="button" class="btn btn-default pull-right" name="Remove" value="Remove"></input>
				</div>
			</div>
			<div class = "col-sm-4">
			<div class = "row">
				<h2>
					Scheduling
				</h2>
				</div>
				<div class = "row">
				<h3>
					Start
				</h3>
				</div>
				<div class="row">
					<input type="date">
					<input id="time" type="text" class="time" />
				</div>
				
				<script>
					$('#time').timepicker({ 'timeFormat': 'h:i A' });
				</script>
				<div class="row">
					<h3> Repeats every <select class='dropdown'><option>1</option><option>2</option><option>3</option><option>4</option><option>5</option></select> days </h3>
				</div>
			</div>
			<div class = "col-sm-4">
				<div class = "row">
				<h2>
					Restore
				</h2>
				</div>
				<div class="row">
					<h3>
						From
					</h3>
				</div>
				<div class="row">
				<div id="t1" class="tableholder">
					<table class="table">
						<tbody>
							<?php
								$fp = fopen("dates.txt", "r");
								$data = "";
								$data = fgets($fp);
								$dirs = explode(',', $data);
								foreach($dirs as $itemName){
									echo "<tr class='dates'><td>$itemName</td></tr>";
								}
							?>
						</tbody>
					</table>
				</div>
				</div>
				<div class="row">
					<h3>
						Folders
					</h3>
				</div>
				<div class="row">
				<div id="t1" class="tableholder">
					<table class="table">
						<tbody id="dirs_to_restore">
							<tr>
								<td>C:\documents\school\..</td>
							</tr>
							<tr>
								<td>C:\music\..</td>
							</tr>
							<tr>
								<td>C:\pictures\vacation\..</td>
							</tr>
							<tr>
								<td>C:\documents\career\..</td>
							</tr>
							<tr>
								<td>C:\history\essay1\..</td>
							</tr>
							<tr>
								<td>C:\games\battlefront\..</td>
							</tr>
							<tr>
								<td>C:\games\call_of_duty\..</td>
							</tr>
							<tr>
								<td>C:\games\minesweeper\records\..</td>
							</tr>
							<tr>
								<td>C:\projects\programming\algorithms\..</td>
							</tr>
							<tr>
								<td>C:\projects\programming\graphics\..</td>
							</tr>
							<tr>
								<td>C:\projects\programming\games\..</td>
							</tr>
							<tr>
								<td>C:\projects\music\edm\..</td>
							</tr>
						</tbody>
					</table>
				</div>
				</div>
				<div class="row">
					<div class="checkbox">
						<label><input type="checkbox" value="">Restore All</label>
					</div>
				</div>
				<div class="row">
					<input id="restore" type="submit" class="btn btn-default" name="Restore Now" value = "Restore Now"></input>
				</div>
			</div>
		</div>
	</div>
</body>
</html>
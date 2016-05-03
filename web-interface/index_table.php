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
	<link href="http://cdn-na.infragistics.com/igniteui/2016.1/latest/css/themes/infragistics/infragistics.theme.css" rel="stylesheet" />
    <link href="http://cdn-na.infragistics.com/igniteui/2016.1/latest/css/structure/infragistics.css" rel="stylesheet" />
	<script src="http://ajax.aspnetcdn.com/ajax/modernizr/modernizr-2.8.3.js"></script>
    <script src="http://code.jquery.com/jquery-1.9.1.min.js"></script>
    <script src="http://code.jquery.com/ui/1.10.3/jquery-ui.min.js"></script>
	<script src="http://cdn-na.infragistics.com/igniteui/2016.1/latest/js/infragistics.core.js"></script>
    <script src="http://cdn-na.infragistics.com/igniteui/2016.1/latest/js/infragistics.lob.js"></script>
	<style>
        .ui-icon.ui-igtreegrid-expansion-indicator.ui-icon-minus {
            background: url(http://www.igniteui.com/images/samples/tree-grid/opened_folder.png) !important;
            background-repeat: no-repeat;
        }
        .ui-icon.ui-igtreegrid-expansion-indicator.ui-icon-plus {
            background: url(http://www.igniteui.com/images/samples/tree-grid/folder.png) !important;
            background-repeat: no-repeat;
        }
		.ui-icon-plus:before {
		    content: '' !important;
		}
		.ui-icon-minus:before{
		    content: '' !important;
		}
    </style>
	<script src="jquery.timepicker.js"></script>
	<script>
	//this function is for the directories to backup table
	
	var dirs_for_date;
	var added = new Array();
	var added_ex = new Array();
	var removed = new Array();
	var removed_ex = new Array();
	var user = "<?php echo $_GET['user']?>";
	var addr = "<?php echo $_SERVER['REMOTE_ADDR']?>";
	var dirs_to_backup_data_string = "<?php 
		$user = $_GET['user'];
		$addr = $_SERVER['REMOTE_ADDR'];
		exec('getBackupDirs $user $addr');
		$fp = fopen("files.txt", "r");
		$data = "";
		while(($line = fgets($fp)) !== false){
			$line = addslashes($line);
			$data .= $line;
		}
		echo $data;
	?>";
	
	$(function() {
		makeClassHighlightable('.dirs_to_backup_class');
		makeClassHighlightable('.excludes_class');
		makeClassHighlightable('.dirs_to_restore_class');
	}
	);
	
	function makeClassHighlightable(Class){
		var rows = $(Class);
		rows.on('click', makeMake(Class));
	}
	
	function makeMake(Class){
		return function(e){
			/* Get current row */
			var row = $(this);
			var rows = $(Class);

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

		}
	}
	
	function makeHighlightable(e) {

			/* Get current row */
			var row = $(this);
			var rows = $('.dirs_to_backup');

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

		}
		
		
		//this function is for the dates table, IS DIFFERENT BECAUSE YOU CAN ONLY SELECT ONE DATE AT A TIME
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
			xhttp.send("date=" + date + "&user=" + user + "&addr=" + addr);
			});

		});
	
		function set_restore_dirs(){
			var new_tbody = document.createElement('tbody');
			var dirs = dirs_for_date.split(",");
			for(var i = 0; i < dirs.length; i++){
				var newRow = new_tbody.insertRow(0);
				newRow.className += 'dirs_to_restore_class';
				var newCell = newRow.insertCell(0);
				var newText = document.createTextNode(dirs[i]);
				newCell.appendChild(newText);
			}
			$(new_tbody).attr("id", "dirs_to_restore");
			var old_tbody = document.getElementById('dirs_to_restore');
			old_tbody.parentNode.replaceChild(new_tbody, old_tbody);
			makeClassHighlightable('.dirs_to_restore_class');
		}
		function add_in_table(t, value, Class){
			var tbody = document.getElementById(t);
			var newRow = tbody.insertRow(0);
			var newCell = newRow.insertCell(0);
			var newText = document.createTextNode(value);
			newCell.appendChild(newText);
			newRow.addEventListener('click', makeMake(Class));
		}
		function add(){
			var new_dir = document.getElementById("add_dir").value;
			if (new_dir == "") return;
			added.push(new_dir);
			add_in_table('dirs_to_backup', new_dir, '.dirs_to_backup_class');
			document.getElementById('add_dir').value = "";
		}
		function add_ex(){
			var new_ex = document.getElementById('add_ex').value;
			if (new_ex == "") return;
			added_ex.push(new_ex);
			add_in_table('excludes', new_ex, '.excludes_class');
			document.getElementById('add_ex').value = "";
		}
		function save(display_msg){
			if(display_msg == undefined){
				display_msg = true;
			}
			var added_string;
			if(added.length == 0) added_string = "";
			else
			{
				added_string = added[0];
			
				var i;
				for(i = 1; i < added.length;i++){
					added_string += ",";
					added_string += added[i];
				}
			}
			
			var removed_string;
			if(removed.length == 0) removed_string = "";
			else{
				removed_string = removed[0];
				var i;
				for(i = 1; i < removed.length; i++){
					removed_string += ",";
					removed_string += added[i];
				}
			}
			
			var added_excludes_string;
			if(added_ex.length == 0) added_excludes_string = "";
			else{
				added_excludes_string = added_ex[0];
				var i;
				for(i = 1; i < added_ex.length; i++){
					added_excludes_string += ",";
					added_excludes_string += added_ex[i];
				}
			}
			
			var removed_excludes_string;
			if(removed_ex.length == 0) removed_excludes_string = "";
			else{
				removed_excludes_string = removed_ex[0];
				var i;
				for(i = 1; i < removed_ex.length; i++){
					removed_excludes_string += ",";
					removed_excludes_string += removed_ex[i];
				}
			}
			
			var date_and_time_defined = true;
			var date = document.getElementById('start_date').value;
			if(date == "") date_and_time_defined = false;
			var time = document.getElementById('time').value;
			if(time == "") date_and_time_defined = false;
			
			date.replace("/", "-");
			
			var am_or_pm = time.toString().slice(6, 8);
			time = time.toString().slice(0,5);
			if(am_or_pm == "PM"){
				var hour = time.slice(0,2);
				var minute = time.slice(2,5);
				
				var new_hour = Number(hour) + 12;
				time = new_hour + minute;
			}
			
			date_and_time = date + " " + time;
			
			if(!date_and_time_defined) date_and_time = "UNDEFINED";
			
			console.log(added_string);
			console.log(removed_string);
			console.log(added_excludes_string);
			console.log(removed_excludes_string);
			console.log(date_and_time);

			
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function(){
				if(xhttp.readyState == 4 && xhttp.status == 200 && display_msg){
					alert("sa 	ve successful");
				}
			};
			xhttp.open("POST", "save.php", true);
			xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			xhttp.send("add=" + added_string + "&remove=" + removed_string + "&add_ex=" + added_excludes_string + "&remove_ex=" + removed_excludes_string + "&dateAndTime" +date_and_time + "&user=" + user + "&addr=" + addr);
			
			added = [];
			removed = [];
			
		}
		function backup_now(){
			save(false);
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function(){
				if(xhttp.readyState == 4 && xhttp.status == 200){
					alert("backup started");
				}
			};
			xhttp.open("POST", "backup_now.php", true);
			xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			xhttp.send("user=" + user + "&addr=" + addr);
		}
		function restore_now(){
			var table0 = document.getElementById('dates_table');
			var date;
			for(var i = 0, row; row = table0.rows[i]; i++){
				if(hasClass(row, 'highlight')){
					date = row.cells[0].innerHTML;
					break;
				}
			}
			var table = document.getElementById('dirs_to_restore');
			var list_of_dirs_to_backup = new Array();
			var restore_all = document.getElementById('restore_all').checked;
			for(var i=0, row; row = table.rows[i]; i++){
				if(hasClass(row, 'highlight') || restore_all){
					var dir = row.cells[0].innerHTML;
					list_of_dirs_to_backup.push(dir);
				}
			}
			var dirs_to_backup_string = "";
			for(var i = 0; i < list_of_dirs_to_backup.length; i++){
				if(i != 0) dirs_to_backup_string += ',';
				dirs_to_backup_string += list_of_dirs_to_backup[i];
			}
			
			console.log(date);
			console.log(dirs_to_backup_string);
			
			if((date == undefined) || (dirs_to_backup_string == "")){
				alert("No directories selected to restore!");
			}
			else{
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function(){
				if(xhttp.readyState == 4 && xhttp.status == 200){
					alert("save successful");
				}
			};
			xhttp.open("POST", "restore.php", true);
			xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			xhttp.send("date=" + date + "&dirs=" + dirs_to_backup_string + "&user=" + user + "&addr=" + addr);
			}
		}
		function remove(){
			var table = document.getElementById('dirs_to_backup');
			for(var i = 0, row; row = table.rows[i]; i++){
				if(hasClass(row, 'highlight')){
					var was_just_added = false;
					var dir_to_remove = row.cells[0].innerHTML;
					for(var j = 0; j < added.length; j++){
						if(added[j] == dir_to_remove) added.splice(j, 1);
						was_just_added = true;
					}
					if(!was_just_added)removed.push(dir_to_remove);
					table.deleteRow(i);
				}
			}
			//go through the list and put all highlighted items into the remove array
			//remove those from the display
			//check if those items are in the added array, if so remove them
		}
		function remove_ex(){
			var table = document.getElementById('excludes');
			for(var i = 0, row; row = table.rows[i]; i++){
				if(hasClass(row, 'highlight')){
					var ex_to_remove = row.cells[0].innerHTML;
					var was_just_added = false;
					for(var j = 0; j < added.length; j++){
						if(added_ex[j] == ex_to_remove) added_ex.splice(j, 1);
						was_just_added = true;
					}
					if(!was_just_added)removed_ex.push(ex_to_remove);
					table.deleteRow(i);
					i--;
				}
			}
			//go through the list and put all highlighted items into the remove array
			//remove those from the display
			//check if those items are in the added array, if so remove them
		}
		function registerHandlers(){
			//document.getElementById("Add").onclick = add();
			$('#Add').on('click', add);
			$('#save').on('click', save);
			$('#Remove').on('click', remove);
			$('#Add_Ex').on('click', add_ex);
			$('#Remove_Ex').on('click', remove_ex);
			$('#backup_now').on('click', backup_now);
			$('#restore').on('click', restore_now);
		}
		
		function hasClass( elem, klass ) {
			return (" " + elem.className + " " ).indexOf( " "+klass+" " ) > -1;
		}
		
		$(function () {
			var all_dirs = dirs_to_backup_data_string;  //TEMPORARY, CHANGE THIS
			var dirs_to_backup = all_dirs.split(",");
			var files = new Array();
			for(var i = 0; i < dirs_to_backup.length; i++){
				var pointer = dirs_to_backup[i].split("\\");
				var current_level = files;
				var current_pointer_dir = 0;
				for(var j = 0; j < current_level.length; j++){
					//check if any added directories are already in the array
					if(current_pointer_dir >= pointer.length){
						i++;
						break;
					}
					else if(pointer[current_pointer_dir] == current_level[j].name){
						//go down a level and begin comparing the next directories
						current_level = current_level[j].files;
						j = 0;
						current_pointer_dir++;
					}
				}
				//if you have now have a new unique pointer path
				if(j == current_level.length){
					while(current_pointer_dir < pointer.length){
						if(pointer[current_pointer_dir] != ".."){
						var new_dir = {name:pointer[current_pointer_dir], files: new Array()};
						current_level.push(new_dir);
						current_level = new_dir.files;
						current_pointer_dir++;
						}
						else{
							break;
						}
					}
				}
			}
			
            $("#dirs_to_backup").igTreeGrid({
                width: "100%",
                height:"700px",
                dataSource: files,
                autoGenerateColumns: false,
                primaryKey: "name",
                columns: [
                    { headerText: "Name", key: "name", width: "250px", dataType: "string" }
                ],
                childDataKey: "files",
                initialExpandDepth: 0,
                features: [
                {
                    name: "Selection",
                    multipleSelection: true
                },
                {
                    name: "RowSelectors",
                    enableCheckBoxes: true,
                    checkBoxMode: "biState",
                    enableSelectAllForPaging: true,
                    enableRowNumbering: false
                },
                {
                    name: "Sorting"
                },
                {
                    name: "Filtering",
                    columnSettings: [
                        {
                            columnKey: "dateModified",
                            condition: "after"
                        },
                        {
                            columnKey: "size",
                            condition: "greaterThan"
                         }
                        ]
                }]
            });
        });
	</script>
</head>
<body onload="registerHandlers()">
	<div class = "container-fluid">
		<div class = "page-header">
			<h1> Backup Settings </h1>
			<input id= "save" type="button" class="btn btn-default " name="Save Settings" value = "Save Settings"></input>
		</div>
			<div class = "col-sm-4">
				<h2>
					Choose Folders
				</h2>
				<div id="dirs_tableholder" class="tableholder">
					<table class="table" id="dirs_to_backup">
					<tbody>
							<?php
								/*$fp = fopen("files.txt", "r");
								$data = "";
								$data = fgets($fp);
								$dirs = explode(',', $data);
								foreach($dirs as $itemName){
									echo "<tr class='dirs_to_backup_class'><td>$itemName</td></tr>";
								}*/
								?>
						</tbody>
					</table>
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
					<input id="start_date" type="date">
					<input id="time" type="text" class="time" />
				</div>
				
				<script>
					$('#time').timepicker({ 'timeFormat': 'h:i A' });
				</script>
				<div class="row">
					<h3> Repeats every <select class='dropdown'><option>1</option><option>2</option><option>3</option><option>4</option><option>5</option><option>6</option><option>7</option><option>8</option><option>9</option><option>10</option><option>11</option><option>12</option><option>13</option><option>14</option></select> <select class='dropdown'><option>hours</option><option>days</option></select></h3>
				</div>
				<div class="centered">
				<div class="row top-buffer">
					<div class="span6" style="float: none; margin; 0 auto;">
					<input id="backup_now" type="button" class="btn btn-default btn-xlarge" name="Backup Now" value = "Backup Now"></input>
					</div>
				</div>
				</div>
				<div class="row">
				<h2>
					Excludes
				</h2>
				</div>
				<div id="t1" class="tableholder">
					<table class="table" id="excludes">
						<tbody>
							<?php
							$user = $_GET['user'];
							$addr = $_SERVER['REMOTE_ADDR'];
							exec('getExcludes $user $addr');
							$fp = fopen("excludes.txt", "r");
							$data = "";
							$data = fgets($fp);
							$dirs = explode(',', $data);
							foreach($dirs as $itemName){
								echo "<tr class='excludes_class'><td>$itemName</td></tr>";
							}
							?>
						</tbody>
					</table>
				</div>
			<input id = "add_ex" class ="text" type="text"/>
			<div class="row folderbuttonrow">
			<input id="Add_Ex" type="button" class="btn btn-default" name="Add" value="Add"></input>
			<input id="Remove_Ex" type="button" class="btn btn-default pull-right" name="Remove" value="Remove"></input>
			</div>
			</div>
			<div class = "col-sm-4">
				<div class = "row">
				<h2>
					Restore from Date
				</h2>
				</div>
				<div class="row">
				<div id="t1" class="tableholder">
					<table class="table" id="dates_table">
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
							<!--<tr>
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
							</tr>-->
						</tbody>
					</table>
				</div>
				</div>
				<div class="row">
					<div class="checkbox">
						<label><input id="restore_all" type="checkbox" value="">Restore All</label>
					</div>
				</div>
				<div class="row">
					<input id="restore" type="submit" class="btn btn-default" name="Restore Now" value = "Restore Now"></input>
				</div>
			</div>
		</div>
</body>
</html>
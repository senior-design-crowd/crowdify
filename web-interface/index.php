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
</head>
<body>
	<div class = "container-fluid">
		<div class = "page-header">
			<h1> Backup Settings </h1>
		</div>
		<div class = "row">
			<div class = "col-sm-4">
				<h2>
					Choose Folders
				</h2>
				<div id="t1" class="tableholder">
					<table class="table table-striped">
						<tbody>
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
				<div class="row folderbuttonrow">
					<input type="submit" class="btn btn-default" name="Add" value = "Add"></input>
					<input type="submit" class="btn btn-default pull-right" name="Remove" value="Remove"></input>
				</div>
				<h2>
					Excludes
				</h2>
				<div id="t1" class="tableholder">
					<table class="table table-striped">
						<tbody>
							<tr>
								<td>..*\.bak</td>
							</tr>
							<tr>
								<td>~..*</td>
							</tr>
						</tbody>
					</table>
					<input type="submit" class="btn btn-default" name="Add" value="Add"></input>
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
					<table class="table table-striped">
						<tbody>
							<tr>
								<td>11/19/2016</td>
							</tr>
							<tr>
								<td>11/16/2016</td>
							</tr>
							<tr>
								<td>11/13/2016</td>
							</tr>
							<tr>
								<td>11/10/2016</td>
							</tr>
							<tr>
								<td>11/13/2016</td>
							</tr>
							<tr>
								<td>11/10/2016</td>
							</tr>
							<tr>
								<td>11/07/2016</td>
							</tr>
							
							<tr>
								<td>11/04/2016</td>
							</tr>
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
					<table class="table table-striped">
						<tbody>
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
					<input type="submit" class="btn btn-default" name="Restore" value = "Restore"></input>
				</div>
			</div>
		</div>
	</div>
</body>
</html>
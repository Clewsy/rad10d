<?php

	// Toggle button (play/pause)
	if(isset($_GET['toggle_button_x']) || isset($_GET['toggle_button_y'])) {
		exec("/usr/bin/mpc toggle");
	}
	
	// Volume buttons
	if(isset($_GET['vol_down_button_x']) || isset($_GET['vol_down_button_y'])) {
		exec("/usr/bin/mpc volume -10");
	}
	if(isset($_GET['vol_up_button_x']) || isset($_GET['vol_up_button_y'])) {
		exec("/usr/bin/mpc volume +10");
	}
	
	// Preset buttons
	if(isset($_GET['triple_j'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add http://live-radio01.mediahubaustralia.com/2TJW/mp3/ && /usr/bin/mpc play");
	}
	if(isset($_GET['coderadio'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add https://coderadio-admin.freecodecamp.org/radio/8010/radio.mp3 && /usr/bin/mpc play");
	}
	if(isset($_GET['proton'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add http://icecast.protonradio.com:8000/schedule && /usr/bin/mpc play");
	}
	if(isset($_GET['nightride_fm'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add https://stream.nightride.fm/nightride.m4a && /usr/bin/mpc play");
	}
	if(isset($_GET['rekt_fm'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add https://stream.nightride.fm/rekt.m4a && /usr/bin/mpc play");
	}

	// Update status
	$PLAYING=exec("mpc current");
	$VOLUME=exec("mpc volume | cut -d\":\" -f2");
	$STATUS=exec("mpc status | cut -s -d\"[\" -f2 | cut -d\"]\" -f1");	//Escape the quotes.
?>

<!DOCTYPE html>
<html>
	<head>
		<meta name="viewport" content="width=device-width" />
		<title>rad10</title>
		<link href="/css/style.css" type="text/css" rel="stylesheet" />
	</head>
	<body>
		<center>
			<form method="get" action="index.php">
				<h1>rad10</h1>
				<input type="image" class="large_button" src="images/toggle.png" name="toggle_button" />
				<br />
				<input type="image" class="small_button" src="images/minus.png" name="vol_down_button" />
				<input type="image" class="small_button" src="images/plus.png" name="vol_up_button" />
				<br />
				<h2>presets</h2>
				<input type="submit" class="text_button" value="triple_j" name="triple_j" />
				<input type="submit" class="text_button" value="coderadio" name="coderadio" />
				<br />
				<input type="submit" class="text_button" value="proton" name="proton" />
				<input type="submit" class="text_button" value="nightride_fm" name="nightride_fm" />
				<br />
				<input type="submit" class="text_button" value="rekt_fm" name="rekt_fm" />
				<br />
			</form>
			<h2>status</h2>
			<table id="status_box">
				<tr>
					<td id="playing" colspan="2"><?php echo $PLAYING; ?></td>
				</tid>
				<tr>
					<td id="status"><?php echo $STATUS; ?></td>
					<td id="volume">volume <?php echo $VOLUME; ?></td>
				</tr>
			</table>
			<a href="/"><img class="small_button" src="images/refresh.png" /></a>
		</center>
	</body>
</html>

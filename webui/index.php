<!DOCTYPE html>
<html>
	<head>
		<meta name="viewport" content="width=device-width" />
		<title>rad10</title>
		<link href="/css/style.css" type="text/css" rel="stylesheet" />
	</head>
	<body>
		<center><h1>rad10</h1>      
			<form method="get" action="index.php">
				<input type="image" class="large_button" src="images/toggle.png" name="toggle_button" />
				<br />
				<input type="image" class="small_button"  src="images/minus.png" name="vol_down_button" />
				<input type="image" class="small_button"  src="images/plus.png" name="vol_up_button" />
				<br />
				<h2>presets</h2>
				<input type="submit" class="text_button" value="triplej" name="triplej" />
				<br />
				<input type="submit" class="text_button" value="coderadio" name="coderadio" />
				<br />
				<input type="submit" class="text_button" value="proton" name="proton" />
				<br />
			</form>
		</center>
	</body>
</html>
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
	if(isset($_GET['triplej'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add http://live-radio01.mediahubaustralia.com/2TJW/aac/ && /usr/bin/mpc play");
	}
	if(isset($_GET['coderadio'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add https://coderadio-admin.freecodecamp.org/radio/8010/radio.mp3 && /usr/bin/mpc play");
	}
	if(isset($_GET['proton'])) {
		exec("/usr/bin/mpc clear && /usr/bin/mpc add http://www.protonradio.com:8000/schedule && /usr/bin/mpc play");
	}
?>

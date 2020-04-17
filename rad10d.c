// rad10d.c - rad10d c code file
// For project details visit https://clews.pro/projects/rad10.html and https://gitlab.com/clewsy/rad10d

#include "rad10d.h"


//Create an mpd "connection" structure and return the address of the struct for use.
static struct mpd_connection *setup_connection(void)
{
	struct mpd_connection *conn;					//Initialise the struct.

	conn = mpd_connection_new(MPD_SERVER, MPD_PORT, MPD_TIMEOUT);	//Settings include mpd server IP address, port # and connection timeout (ms).
	if (conn == NULL) {exit(EXIT_FAILURE);}				//Exit on failure to initialise mpd connection.
	return(conn);							//Return the address of the connection struct.
}


//Initialise the mpd connection.
bool init_mpd(void)
{
	connection = setup_connection();			//Initialise mpd connection session.
	if(connection == NULL) {return(FALSE);}			//Confirm "connection" was successfully set up.  If not, return false.
	mpd_connection_set_keepalive(connection, TRUE);		//Enable TCP keepalives to prevent disconnection in the event of a long idle duration.

	return(TRUE);						//Apparently connection has been successfully set up so return true.
}


//Attempt to close then restablish an mpd connection.
bool mpd_reconnect(void)
{
	mpd_connection_free(connection);		//Attempt to close the mpd connection and free the memory.
	if (init_mpd() == FALSE) {return(FALSE);};	//Attempt to re-initialise a new (identical) mpd connection.

	return(TRUE);					//Connection should be successfully re-established so return true.
}


//Return the current mpd status. Possible return values:
//MPD_STATE_UNKNOWN     = 0	no information available
//MPD_STATE_STOP        = 1	not playing
//MPD_STATE_PLAY        = 2	playing
//MPD_STATE_PAUSE       = 3	playing, but paused
uint8_t get_mpd_status(void)
{
	struct mpd_status *status_struct = mpd_run_status(connection);	//Create a struct "status_struct" and fill it with status info of "connection".
	int status = mpd_status_get_state(status_struct);		//Pull the unknown/stop/play/pause status from "status_struct".
	while ((status < MPD_STATE_STOP) || (status > MPD_STATE_PAUSE))	//While the status returned is anything other than STOP, PLAY or PAUSE...
	{
		while (mpd_reconnect() == FALSE) {};			//Attempt re-establishing the mpd connection.
		status = mpd_status_get_state(status_struct);		//Re-check the current status.
	}
	mpd_status_free(status_struct);					//Release "status_struct" from the heap.

	return(status);							//Return the mpd status.
}


//Function to initialise the struct that reflects the state of the encoder.
struct encoder *init_encoder(uint8_t channel_a, uint8_t channel_b)
{
	the_encoder = malloc(sizeof(struct encoder));	//Allocates heap memory for globally accesible "the_encoder".

	the_encoder->channel_a = channel_a;	//Define the encoder channel a pin.
	the_encoder->channel_b = channel_b;	//Define the encoder channel b pin.
	the_encoder->last_code = 0;		//Initialise the last encoder code (for comparing to current code).
	the_encoder->volume_delta = 0;		//Initialise the value of desired volume change.

	if (gpioSetPullUpDown(channel_a, PI_PUD_UP) != 0) {return(NULL);}			//Enable pull-up resistor on channel a pin.
	if (gpioSetPullUpDown(channel_b, PI_PUD_UP) != 0) {return(NULL);}			//Enable pull-up resistor on channel b pin.
	if (gpioSetISRFunc(channel_a, EITHER_EDGE, 100, &volume_ISR) !=0) {return(NULL);}	//Enable hardware interrupt input on encoder channel a pin and define routine to run.
	if (gpioSetISRFunc(channel_b, EITHER_EDGE, 100, &volume_ISR) !=0) {return(NULL);}	//Enable hardware interrupt input on encoder channel b pin and define routine to run.

	return(the_encoder);	//Return address of "*the_encoder".
}


//Interrupt sub-routine that is triggered by any change on either encoder pin.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick.  These values are ignored in this use-case.
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick)
{
	bool MSB = gpioRead(the_encoder->channel_a);		//Define most-significant bit from 2-bit encoder.
	bool LSB = gpioRead(the_encoder->channel_b);		//Define least-significant bit from 2-bit encoder.

	int code = (MSB << 1) | LSB;				//Define 2-bit value read from encoder.
	int sum = (the_encoder->last_code << 2) | code;		//Define 4-bit value: 2 msb is the previous value from the encoder and 2 lsb is the current.

	//Use the following two lines to register every single pulse.
	//That's 4 pulses per detent on ADA377 encoder.
	//if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) the_encoder->value++;	//Turned clockwise.
	//if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) the_encoder->value--;	//Turned counter-clockwise.

	//Use the following two lines to register only every fourth pulse.
	//That's equivalent to a single pulse for every detent on ADA377 encoder.
	if (sum == 0b0100 || sum == 0b1011) {the_encoder->volume_delta++;}	//Turned clockwise.
	if (sum == 0b0111 || sum == 0b1000) {the_encoder->volume_delta--;}	//Turned counter-clockwise.

	the_encoder->last_code = code;	//Update value of last_code for comparison the next time this ISR is triggered.
}


//Interrupt subroutine to be triggered by pressing the toggle button.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick.
void toggle_ISR(int32_t gpio, int32_t level, uint32_t tick)
{
	if (((tick - last_button_trigger) > DEBOUNCE_US) && (gpioRead(TOGGLE_PIN) == HIGH))
	{
		if (block_toggle)	{block_toggle = FALSE;}		//Ignore the toggle if the button was used to send a stop signal.
		else			{toggle_signal = TRUE;}		//Flag toggle with a normal press.

		last_button_trigger = tick;				//Reset the de-bounce timestamp.
	}
}


//Function returns true if the toggle button has been held down for a specified duration (TOGGLE_BUTTON_LONG_PRESS microseconds).
bool toggle_long_press(void)
{
	if (gpioRead(TOGGLE_PIN) == LOW)						//If the button is pressed.
	{
		if (toggle_button_held == FALSE)					//If this is the first iteration when the button has been pressed.
		{
			time_when_pressed = gpioTick();					//Record the time the press was first flagged.
			toggle_button_held = TRUE;					//Set the flag so that the next iteration doesn't reset the timer.
		}

		if ((gpioTick() - (time_when_pressed) > TOGGLE_BUTTON_LONG_PRESS))	//If the button is held for duration exceeding TOGGLE_BUTTON_LONG_PRESS
		{
			block_toggle = TRUE;						//Set the stop flag (so button ISR ignores when the button is released).
			return(TRUE);							//Return true: button long press true.
		}
	}
	else {toggle_button_held = FALSE;}						//Clear the flag that indicates the button is being held.

	return(FALSE);									//Return false: button long press false.
}


//Initialise the hardware inputs - two channels on the rotary encoder and a push-button.
bool init_hardware(void)
{
	if (gpioInitialise() == PI_INIT_FAILED) {return(FALSE);}				//Initialise gpio pins on raspi, return false if failure.

	if (gpioSetMode(TOGGLE_PIN, PI_INPUT) != 0) {return(FALSE);}				//Set as input the pin to which the toggle button is connected.
	if (gpioSetPullUpDown(TOGGLE_PIN, PI_PUD_UP) != 0) {return(FALSE);}			//Enable internal pull-up resistor on the toggle pin.
	if (gpioSetISRFunc(TOGGLE_PIN, RISING_EDGE, 0, &toggle_ISR) != 0) {return(FALSE);}	//Set up interrupt on the toggle button.

	the_encoder = init_encoder(VOL_ENCODER_A_PIN, VOL_ENCODER_B_PIN);			//Initialise values stored in global struct "the_encoder".
	if (the_encoder == NULL) {return(FALSE);}						//Return false if initialisation fails.

	return(TRUE);
}


//Main program loop.  arguments (argv[]) and number of arguments (argc) ignored.
int main(int argc, char* argv[])
{
	////////////////////////////////////////////////////////////////////////////////
	////////	This initial code in main required for daemonisation	////////
	////////////////////////////////////////////////////////////////////////////////

	pid_t pid = 0;	//Initialise pid (process id).
	pid_t sid = 0;	//Initialise sis (session id).

	pid = fork();					//Fork off from the parent process.  fork() should return PID of child process.
	if (pid < 0) {exit(EXIT_FAILURE);}		//If fork returns a value less than 0 then an error has occured.
	if (pid > 0) {exit(EXIT_SUCCESS);}		//if fork returns a positive value then we have successfully forked a child process from the parent process.

	umask(0);					//Unmask the file mode - allow the daemon to open, read and write any file anywhere.

	sid = setsid();					//Set new session and allocate session id.  if successful, child process is now process group leader.
	if(sid < 0) {exit(EXIT_FAILURE);}		//If setsid failed, exit.

	if ((chdir("/")) < 0) {exit(EXIT_FAILURE);} 	//Change working directory. If we can't find the directory we exit with failure.

	//Daemons should not involve any standard user interaction, so close stdin, stdout and stderr.
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	////////////////////////////////////////////////////////////////////////////////////////////////
	////////	The Payload - at this point, the programme is running as a daemon	////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if (init_hardware() != TRUE) {exit(EXIT_FAILURE);}	//Initialise the hardware (encoder and push button).
	if (init_mpd() != TRUE) {exit(EXIT_FAILURE);}		//Initialise mpd connection.

	int current_status;		//Initialise integer to indicate current mpd playing status.

	//The main infinite loop.
	while (TRUE)
	{
		gpioDelay(IDLE_DELAY);			//A delay so as to not max out cpu usage when running the main loop.
		current_status = get_mpd_status();	//Regularly checking mpd status prevents the connection being dropped. 

		//If statement triggered if there is a desired volume change - i.e. the encoder has rotated.
		if (the_encoder->volume_delta)
		{
			//Adjust the volume by an amount equal to the difference between the encoder value and the last encoder value.
			mpd_run_change_volume(connection, the_encoder->volume_delta);	//Change the volume.
			the_encoder->volume_delta = 0;					//Clear the desired volume delta.
		}

		//If the toggle button has been held for an extended duration, send the stop command to mpd.
		if (toggle_long_press()) {mpd_run_stop(connection);}

		//If statement to poll the state of the toggle_signal flag.  A set flag indicates button has been pressed to request play/pause toggle.
		if (toggle_signal)
		{
			if (current_status == MPD_STATE_PLAY)	{mpd_run_pause(connection, TRUE);}	//Currently playing, so pause.
			else					{mpd_run_play(connection);}		//Currently paused or stopped, so play.

			toggle_signal = FALSE;
		}

		//Check for (and try to recover from) connection errors.
		if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS)	//If an error event has occurred.
		{
			mpd_connection_clear_error(connection);			//Attempt to clear any error that may arise.
			while (mpd_reconnect() == FALSE) {}			//Attempt to re-establish an mpd connection.
		}
	}

	return(-1);	//Never reached.
}

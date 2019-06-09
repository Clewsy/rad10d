// rad10d.c - rad10d c file
// for project deatils visit https://gitlab.com/clewsy/rad10d
#include "rad10d.h"

//create an mpd "connection" structure and return the address of the struct for use.
static struct mpd_connection *setup_connection(void)
{
	struct mpd_connection *conn;					//initialise the struct.

	conn = mpd_connection_new(MPD_SERVER, MPD_PORT, MPD_TIMEOUT);	//settings include IP address of mpd server, port # and connection timeout (ms).
	if (conn == NULL) { exit(EXIT_FAILURE);	}			//exit on failure to initialise mpd connection.
	return(conn);							//return the address of the connection struct.
}


//initialise the mpd connection.
bool init_mpd(void)
{
	connection = setup_connection();			//initialise mpc connection session.
	if(connection == NULL) { return(FALSE); }		//confirm "connection" was successfully set up.  if not, return false.
	mpd_connection_set_keepalive(connection, TRUE);		//enable TCP keepalives to prevent disconnection in the event of a long idle duration.

	return(TRUE);						//apparently connection has been successfully set up so return true.
}


//attempt to close an existing mpd connection then establish a new one.
bool mpd_reconnect(void)
{
	mpd_connection_free(connection);		//attempt to close the mpd connection and free the memory.
	if (init_mpd() == FALSE) { return(FALSE); };	//attempt to re-initialise a new (identical) mpd connection.
	if (connection == NULL) { return(FALSE); }	//verify if the connection was successfully created.  if not, return false.

	return(TRUE);					//connection should be successfully re-established so return true.
}


//return the current mpd status. returns:
//MPD_STATE_UNKNOWN     no information available
//MPD_STATE_STOP        not playing
//MPD_STATE_PLAY        playing
//MPD_STATE_PAUSE       playing, but paused
int get_mpd_status(void)
{
	struct mpd_status *status_struct = mpd_run_status(connection);	//creates a struct "status_struct" and fills it with status info of "connection".
	int status = mpd_status_get_state(status_struct);		//pulls the unknown/stop/play/pause status from "status_struct".
	mpd_status_free(status_struct);					//releases "status_struct" from the heap.

	return(status);							//returns the mpd status.
}


//function to initialise the struct that reflects the state of the encoder.
struct encoder *init_encoder(int channel_a, int channel_b)
{
	the_encoder = malloc(sizeof(struct encoder));			//allocates heap memory for globally accesible "the_encoder".

	the_encoder->channel_a = channel_a;				//defines the encoder channel a pin.
	the_encoder->channel_b = channel_b;				//defines the encoder channel b pin.
	the_encoder->value = 0;						//initialises the encoder value.
	the_encoder->last_value = 0;					//initialises the last encoder value (for comparing to current value).
	the_encoder->last_code = 0;					//initialises last_code value (to recalculate value).

	pullUpDnControl(channel_a, PUD_UP);				//enable pull-up resistor on channel a pin.
	pullUpDnControl(channel_b, PUD_UP);				//enable pull-up resistor on channel b pin.
	wiringPiISR(channel_a, INT_EDGE_BOTH, updateEncoderISR);	//enable hardware interrupt input on encoder channel a pin and define routine to run.
	wiringPiISR(channel_b, INT_EDGE_BOTH, updateEncoderISR);	//enable hardware interrupt input on encoder channel b pin and define routine to run.

	return(the_encoder);	//returns address of "*the_encoder".
}


//interrupt sub-routine that is triggered by any change on the encoder pins.
void updateEncoderISR(void)
{
	bool MSB = digitalRead(the_encoder->channel_a);		//define most-significant bit from 2-bit encoder.
	bool LSB = digitalRead(the_encoder->channel_b);		//define least-significant bit from 2-bit encoder.

	int code = (MSB << 1) | LSB;				//define 2-bit value read from encoder.
	int sum = (the_encoder->last_code << 2) | code;		//define 4-bit value: 2 msb is the previous value from the encoder and 2 lsb is the current.

	//use the following two lines are required to register every single pulse.
	//that's 4 pulses per detent on ADA377 encoder.
	//if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) the_encoder->value++;
	//if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) the_encoder->value--;

	//use the following two lines to register only every fourth pulse.
	//that's equivalent to a single pulse for every detent on ADA377 encoder.
	if (sum == 0b0100 || sum == 0b1011) the_encoder->value++;
	if (sum == 0b0111 || sum == 0b1000) the_encoder->value--;

	the_encoder->last_code = code;	//update value of last_code for use next time this ISR is triggered.
}


//interrupt subroutine to be triggered by pressing the toggle button.
void toggleISR(void)
{
	delay(DEBOUNCE_MS);						//wait for a "debounce" duration.

	if (digitalRead(TOGGLE_PIN) == 0)				//check if the toggle button is still pushed.
	{
		while (mpd_reconnect() == FALSE) {};			//reconnect mpd (in case it has dropped after a long idle).

		if (	(get_mpd_status() == MPD_STATE_UNKNOWN) ||	//check if status is unknown OR
			(get_mpd_status() == MPD_STATE_STOP) ||		//stopped OR
			(get_mpd_status() == MPD_STATE_PAUSE)	)	//paused.
		{
			if (mpd_run_play(connection) != TRUE)		//send the run_play command.
			{
				while (mpd_reconnect() == FALSE) {};	//if the command fails, try reconnecting.
				mpd_run_play(connection);		//then try the command again.
			}
		}
		else							//the status here should be MPD_STATE_PLAY.
		{
			if (mpd_run_pause(connection, TRUE) != TRUE)	//send the pause command (this is actually a play/pause toggle).
			{
				while (mpd_reconnect() == FALSE) {};	//if the command fails, try reconnecting.
				mpd_run_pause(connection, TRUE);	//then try the command again.
			}
		}
		while(digitalRead(TOGGLE_PIN) == 0) { }			//infinite loop until toggle button is released.
	}
}


//initialise the hardware inputs - two channels on the rotary encoder and a push-button.
bool init_hardware(void)
{
	if (wiringPiSetup() < 0) { return(FALSE); }						//initialise gpio pins on raspi, return false if failure.

	pinMode(TOGGLE_PIN, INPUT);								//set as input the pin to which the toggle button is connected.
	pullUpDnControl(TOGGLE_PIN, PUD_UP);							//enable internal pull-up resistor on the toggle pin.
	if (wiringPiISR(TOGGLE_PIN, INT_EDGE_FALLING, &toggleISR) < 0) { return(FALSE); }	//set up interrupt on the toggle button, return false if fails.

	the_encoder = init_encoder(VOL_ENCODER_A_PIN, VOL_ENCODER_B_PIN);			//initialise values stored in global struct "the_encoder".
	if (the_encoder == NULL) { return(FALSE); }						//return false if initialisation fails.

	return(TRUE);
}


//main program loop.  arguments (argv[]) and number of arguments (argc) ignored.
int main(int argc, char* argv[])
{
	////////////////////////////////////////////////////////////////////////////////
	////////	This initial code in main required for daemonisation	////////
	////////////////////////////////////////////////////////////////////////////////

	pid_t pid = 0;	//initialise pid (process id)
	pid_t sid = 0;	//initialise sis (session id)

	pid = fork();	//fork off rom the parent process.  fork() should return PID of child process
	if (pid < 0)	//if fork returns a value less than 0 then an error has occured
	{
		exit(EXIT_FAILURE);	//exit the parent process but return failure cose
	}
	if (pid > 0)	//if fork returns a positive value then we have successfully forked a child process from the parent process
	{
		exit(EXIT_SUCCESS);	//exit the parent process successfully
	}
	umask(0);	//unmask the file mode - allow the daemon to open, read and write any file anywhere

	sid = setsid();				//set new session and allocate session id.  if successful, child process is now process group leader
	if(sid < 0) { exit(EXIT_FAILURE); }	//if setsid fails, exit

	//Change working directory. If we cant find the directory we exit with failure.
	if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

	//daemons should not involve any standard user interaction, so close stdin, stdout and stderr
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	////////////////////////////////////////////////////////////////////////////////////////////////
	////////	The Payload - at this point, the programme is running as a daemon	////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if (init_hardware() != TRUE) { exit(EXIT_FAILURE); }
	if (init_mpd() != TRUE) {exit(EXIT_FAILURE); }

	//the main infinite loop.
	while (TRUE)
	{
		delay(10);	//a short delay so as to not max out cpu usage when running the main loop.

		//if statement to poll the encoder for change.
		//the encoder ISR can't be used to directly run the change_volume command because multiple, rapid encoder changes exceed the command buffer.
		if (the_encoder->last_value != the_encoder->value)	//if the encoder value has changed since last checked.
		{
			//adjust the volume by an amount equal to the difference between the encoder value and the last encoder value.
			mpd_run_change_volume(connection, (the_encoder->value - the_encoder->last_value));
			the_encoder->last_value = the_encoder->value;	//update known encoder value for next comparison.
		}

		//check for (and try to recover from) connection errors.
		if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS)	//If an error event has occurred.
		{
			mpd_connection_clear_error(connection);			//attempt to clear any error that may arise.
			while (mpd_reconnect() == FALSE) {}			//attempt to re-establish an mpd connection.
		}
	}

	return(-1);	//never reached.
}

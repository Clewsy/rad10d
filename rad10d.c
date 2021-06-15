//rad10d.c - rad10d c code file
//For project details visit https://clews.pro/projects/rad10.php and https://gitlab.com/clewsy/rad10d
//For info regarding the mpd c library (libmpdclient), visit https://www.musicpd.org/libs/libmpdclient/.

#include "rad10d.h"


//Global declarations.
int8_t volume_delta = 0;			//Desired volume delta set by interrupt and polled to control mpd.
uint8_t mpd_control_signal = SIGNAL_NULL;	//Signal set by interrupt and polled to control mpd.
struct button_timer button_time = {0, 0};	//Button press/release timestamps.
struct mpd_connection *connection = NULL;	//Structure containing mpd connection data.


//Create an mpd "connection" structure and return the address of the struct for use.
static struct mpd_connection *setup_connection(void)
{
	struct mpd_connection *conn;					//Initialise the struct.

	conn = mpd_connection_new(MPD_SERVER, MPD_PORT, MPD_TIMEOUT);	//Define mpd server IP, port # and connection timeout (ms).
	if (conn == NULL) exit(EXIT_FAILURE);				//Exit on failure to initialise mpd connection.

	return(conn);							//Return the address of the connection struct.
}


//Initialise the mpd connection.  Note variable "connection" is a pointer to a struct.
bool init_mpd(void)
{
	connection = setup_connection();		//Initialise mpd connection session.
	if(connection == NULL) return(FALSE);		//Confirm "connection" was successfully set up.  If not, return false.
	mpd_connection_set_keepalive(connection, TRUE);	//Enable TCP keepalives to prevent idle disconnection.

	return(TRUE);					//Apparently connection has been successfully established so return true.
}


//Attempt to close then restablish an mpd connection.
bool mpd_reconnect(void)
{
	mpd_connection_free(connection);	//Close the mpd connection and free the memory.
	if (init_mpd() == FALSE) return(FALSE);	//Attempt to re-initialise a new (identical) mpd connection.

	return(TRUE);				//Connection should be successfully re-established so return true.
}


//Return the current mpd status. Possible return values:
//MPD_STATE_UNKNOWN     = 0	no information available
//MPD_STATE_STOP        = 1	not playing
//MPD_STATE_PLAY        = 2	playing
//MPD_STATE_PAUSE       = 3	playing, but paused
uint8_t get_mpd_status(void)
{
	struct mpd_status *status_struct = mpd_run_status(connection);	//Create "status_struct" (contains status of "connection").
	int status = mpd_status_get_state(status_struct);		//Pull the current status from "status_struct".

	while ((status < MPD_STATE_STOP) || (status > MPD_STATE_PAUSE))	//While the status is anything but STOP, PLAY or PAUSE...
	{
		while (mpd_reconnect() == FALSE) {};			//Attempt re-establishing the mpd connection.
		status = mpd_status_get_state(status_struct);		//Re-check the current status.
	}
	mpd_status_free(status_struct);					//Release "status_struct" from the heap.

	return(status);							//Return the mpd status.
}


//Function to initialise the struct that reflects the state of the encoder hardware.  Returns address of the initialised struct.
//struct encoder *init_encoder(void)
bool init_encoder(void)
{
	if (gpioSetMode(VOL_ENCODER_A_PIN, PI_INPUT)) return(FALSE);	//Set encoder channel a pin as input.
	if (gpioSetMode(VOL_ENCODER_B_PIN, PI_INPUT)) return(FALSE);	//Set encoder channel b pin as input.
	if (gpioSetMode(BUTTON_PIN, PI_INPUT)) return(FALSE);		//Set button pin as input.

	if (gpioSetPullUpDown(VOL_ENCODER_A_PIN, PI_PUD_UP)) return(FALSE);	//Enable pull-up resistor on channel a pin.
	if (gpioSetPullUpDown(VOL_ENCODER_B_PIN, PI_PUD_UP)) return(FALSE);	//Enable pull-up resistor on channel b pin.
	if (gpioSetPullUpDown(BUTTON_PIN, PI_PUD_UP)) return(FALSE);		//Enable pull-up resistor on push-button pin.

	if (gpioSetISRFunc(VOL_ENCODER_A_PIN, EITHER_EDGE, 100, &volume_ISR)) return(FALSE);	//Define ISR ttriggered by a pin.
	if (gpioSetISRFunc(VOL_ENCODER_B_PIN, EITHER_EDGE, 100, &volume_ISR)) return(FALSE);	//Define ISR ttriggered by b pin.
	if (gpioSetISRFunc(BUTTON_PIN, EITHER_EDGE, 0, &button_ISR)) return(FALSE);		//Define ISR ttriggered by button.

	return(TRUE);	//Initialise was successful.
}


//Interrupt sub-routine that is triggered by any change on either encoder pin.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick.  These values are ignored in this use-case.
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick)
{
	static uint8_t last_code = 0;	//Static so that value is remembered every time this ISR is triggered,

	bool msb = gpioRead(VOL_ENCODER_A_PIN);	//Define most-significant bit from 2-bit encoder.
	bool lsb = gpioRead(VOL_ENCODER_B_PIN);	//Define least-significant bit from 2-bit encoder.

	uint8_t code = (msb << 1) | lsb;	//Read 2-bit value read from encoder.
	uint8_t abab = (last_code << 2) | code;	//Set 4-bit value:	2 msb : previous value from the encoder.
						//			2 lsb : current value from the encoder.

	//State table for determining rotation direction based on previous and current encoder channel values.
	//  +--------------Previous reading.
	//  |    +---------Current reading.
	//  |    |    +----Combined binary value (variable abab).
	//  |    |    |
	//| AB | AB | ABAB | direction | volume_delta |
	//| 00 | 00 | 0000 |   none    |      0       |
	//| 00 | 01 | 0001 |   ccw     |     -1       |
	//| 00 | 10 | 0010 |   cw      |      1       |
	//| 00 | 11 | 0011 |   error   |      0       |
	//| 01 | 00 | 0100 |   cw      |      1       |
	//| 01 | 01 | 0101 |   none    |      0       |
	//| 01 | 10 | 0110 |   error   |      0       |
	//| 01 | 11 | 0111 |   ccw     |     -1       |
	//| 10 | 00 | 1000 |   ccw     |     -1       |
	//| 10 | 01 | 1001 |   error   |      0       |
	//| 10 | 10 | 1010 |   none    |      0       |
	//| 10 | 11 | 1011 |   cw      |      1       |
	//| 11 | 00 | 1100 |   error   |      0       |
	//| 11 | 01 | 1101 |   cw      |      1       |
	//| 11 | 10 | 1110 |   ccw     |     -1       |
	//| 11 | 11 | 1111 |   none    |      0       |

	//Progression of AB when turning clockwise:
	//00 -> 10 -> 11 -> 01 -> 00
	//Progression of AB when turning counter-clockwise:
	//00 -> 01 -> 11 -> 10 -> 00

	//Use the following two lines to register every single pulse.
	//That's 4 pulses per detent on ADA377 encoder.
	//if(abab == 0b0010 || abab == 0b1011 || abab == 0b1101 || abab == 0b0100) volume_delta++;	//Turned clockwise.
	//if(abab == 0b0001 || abab == 0b0111 || abab == 0b1110 || abab == 0b1000) volume_delta--;	//Turned counter-clockwise.

	//Use the following two lines to register only every second pulse.
	//That's equivalent to two pulses for every detent on ADA377 encoder.
	if (abab == 0b0010 || abab == 0b1101) volume_delta++;	//Turned clockwise.
	if (abab == 0b0001 || abab == 0b1110) volume_delta--;	//Turned counter-clockwise.

	last_code = code;	//Update value of last_code for comparison the next time this ISR is triggered.
}


//Interrupt subroutine to be triggered by pressing (or releasing) the toggle button.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick (renamed time).
//Note, action is only taken when the button is released.
void button_ISR(int32_t gpio, int32_t level, uint32_t time)
{
	//If button is pressed, record the current timestamp as button_time.pressed.
	if (gpioRead(BUTTON_PIN) == LOW) button_time.pressed = time;

	//If button is not pressed and a time greater than the debounce has elapsed since button was pressed.
	if ((gpioRead(BUTTON_PIN) == HIGH) && ((time - button_time.released) > DEBOUNCE_US))
	{
		//A legitimate button press has been detected (debounced), so set the desired signal (triggers action in main).
		if (mpd_control_signal == SIGNAL_STOP)	mpd_control_signal = SIGNAL_NULL;	//Set null signal.
		else					mpd_control_signal = SIGNAL_TOGGLE;	//Set toggle signal.

		button_time.released = time;	//Reset the button release timestamp.
	}
}


//Function returns true if the toggle button has been held down for a specified duration (TOGGLE_BUTTON_LONG_PRESS microseconds).
void poll_long_press(uint8_t *signal_address)
{
	//If the button is pressed and has been pressed for a duration greater than the period that definbes a "long press".
	if	((gpioRead(BUTTON_PIN) == LOW) && ((gpioTick() - button_time.pressed) > TOGGLE_BUTTON_LONG_PRESS)) 
	{
		*signal_address = SIGNAL_STOP; //Set stop signal.  Cleared by button_ISR when button is released.
	}
}


//Triggered in the main loop by polling the toggle flag (set by ISR).
void update_mpd_state(uint8_t *signal_address)
{	
	switch(*signal_address)
	{
		case SIGNAL_TOGGLE:
			if (get_mpd_status() == MPD_STATE_PLAY)	mpd_run_pause(connection, TRUE);	//Playing, so pause.
			else					mpd_run_play(connection);		//Paused/stopped, so play.
			*signal_address = SIGNAL_NULL;							//Reset mpd state signal.
			break;
		case SIGNAL_STOP:
			mpd_run_stop(connection);	//Send the stop command.
			break;				//Do not reset signal, used as a check in the button ISR.
	}
}


//Triggered in the main loop by polling for a change in the encoder value.
void update_mpd_volume(int8_t *delta_address)
{
	mpd_run_change_volume(connection, *delta_address);	//Change the volume.
	*delta_address = 0;					//Clear the desired volume delta.
}


//Initialise the hardware inputs - two channels on the rotary encoder and a push-button.
bool init_hardware(void)
{
	if (gpioInitialise() == PI_INIT_FAILED) return(FALSE);	//Initialise gpio pins on raspi.
	if (!init_encoder()) return(FALSE);			//Configure gpio pins connected to physical encoder/button.

	return(TRUE); //Successfully initialised.
}


//Main program loop.  arguments (argv[]) and number of arguments (argc) ignored.
int main(int argc, char *argv[])
{
	////////////////////////////////////////////////////////////////////////////////
	////////	This initial code in main required for daemonisation	////////
	////////////////////////////////////////////////////////////////////////////////

	pid_t pid = 0;	//Initialise pid (process id).
	pid_t sid = 0;	//Initialise sid (session id).

	pid = fork();					//Fork off from the parent process.  fork() should return PID of child.
	if (pid < 0)		exit(EXIT_FAILURE);	//If fork returns a value less than 0 then an error has occured.
	if (pid > 0)		exit(EXIT_SUCCESS);	//if fork returns a positive value then we have successfully forked.

	umask(0);					//Unmask the file mode (allow daemon to open, read and write any file).

	sid = setsid();					//Set new session and allocate session id.  
							//If successful, child process is now process group leader.
	if (sid < 0)		exit(EXIT_FAILURE);	//Exit if setsid failed, otherwise child is now process group leader.

	if ((chdir("/")) < 0)	exit(EXIT_FAILURE);	//Change working directory. If can't find the directory, exit with failure.

	//Daemons should not involve any standard user interaction, so close stdin, stdout and stderr.
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	////////////////////////////////////////////////////////////////////////////////////////////////
	////////	The Payload - at this point, the programme is running as a daemon	////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if (!init_hardware())	exit(EXIT_FAILURE);	//Initialise the hardware (encoder and push button).
	if (!init_mpd())	exit(EXIT_FAILURE);	//Initialise mpd connection.

	//The main infinite loop.
	while (TRUE)
	{
		get_mpd_status();	//Loop until valid connection state. Keeps the connection alive.
		gpioDelay(IDLE_DELAY);	//A delay so as to not max out cpu usage when running the main loop.

		poll_long_press(&mpd_control_signal);				//Poll for button hold (will send stop signal).
		if (volume_delta)	update_mpd_volume(&volume_delta);	//Poll for encoder change (volume delta).
		if (mpd_control_signal)	update_mpd_state(&mpd_control_signal);	//Poll mpd control signal (toggle/stop).
	}

	return(-1);	//Never reached.
}

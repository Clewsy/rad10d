//rad10d.c - rad10d c code file
//For project details visit https://clews.pro/projects/rad10.php and https://gitlab.com/clewsy/rad10d
//For info regarding the mpd c library (libmpdclient), visit https://www.musicpd.org/libs/libmpdclient/.

#include "rad10d.h"

//Global declarations.
struct encoder *the_encoder;			//Declare structure named the_encoder - gloablly accessible (used in ISR).
struct mpd_connection *connection = NULL;	//Initialise globally accessible structure containing mpd connection.


//Create an mpd "connection" structure and return the address of the struct for use.
static struct mpd_connection *setup_connection(void)
{
	struct mpd_connection *conn;					//Initialise the struct.

	conn = mpd_connection_new(MPD_SERVER, MPD_PORT, MPD_TIMEOUT);	//Define mpd server IP, port # and connection timeout (ms).
	if (conn == NULL) exit(EXIT_FAILURE);				//Exit on failure to initialise mpd connection.

	return(conn);							//Return the address of the connection struct.
}


//Initialise the mpd connection.
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


//Function to initialise the struct that reflects the state of the encoder hardware.
struct encoder *init_encoder(const uint8_t channel_a, const uint8_t channel_b, const uint8_t button_pin)
{
	the_encoder = malloc(sizeof(struct encoder));	//Allocates heap memory for globally accessible "the_encoder".

	//Allocate memory and initialise values of the encoder structure
	the_encoder->channel_a = channel_a;		//Encoder channel a hardware pin.
	the_encoder->channel_b = channel_b;		//Encoder channel b hardware pin.
	the_encoder->last_code = 0;			//Last encoder code (for comparing to current code).
	the_encoder->volume_delta = 0;			//Value of desired volume change.

	the_encoder->button_pin = button_pin;		//Define the push-button hardware pin.
	the_encoder->button_pressed_time = 0;		//Initialise time when button press (low) is detected and validated.
	the_encoder->button_released_time = 0;		//Initialise time when button release (high) is detected and validated.
	the_encoder->button_signal = SIGNAL_NULL;	//Initialise the control signal which is set based on button state.
							//(quick press to toggle, long press to stop).

	if (gpioSetMode(channel_a, PI_INPUT)) return(FALSE);	//Set as input the pin to which the encoder channel a is connected.
	if (gpioSetMode(channel_a, PI_INPUT)) return(FALSE);	//Set as input the pin to which the encoder channel b is connected.
	if (gpioSetMode(button_pin, PI_INPUT)) return(FALSE);	//Set as input the pin to which the push-button is connected.

	if (gpioSetPullUpDown(channel_a, PI_PUD_UP)) return(FALSE);	//Enable pull-up resistor on channel a pin.
	if (gpioSetPullUpDown(channel_b, PI_PUD_UP)) return(FALSE);	//Enable pull-up resistor on channel b pin.
	if (gpioSetPullUpDown(button_pin, PI_PUD_UP)) return(FALSE);	//Enable pull-up resistor on push-button pin.

	if (gpioSetISRFunc(channel_a, EITHER_EDGE, 100, &volume_ISR)) return(FALSE);	//Define routine to run on channel a int.
	if (gpioSetISRFunc(channel_b, EITHER_EDGE, 100, &volume_ISR)) return(FALSE);	//Define routine to run on channel b int.
	if (gpioSetISRFunc(button_pin, EITHER_EDGE, 0, &button_ISR)) return(FALSE);	//Define routine to run on button int.

	return(the_encoder);	//Return address of "*the_encoder".
}


//Interrupt sub-routine that is triggered by any change on either encoder pin.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick.  These values are ignored in this use-case.
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick)
{
	bool MSB = gpioRead(the_encoder->channel_a);	//Define most-significant bit from 2-bit encoder.
	bool LSB = gpioRead(the_encoder->channel_b);	//Define least-significant bit from 2-bit encoder.

	int code = (MSB << 1) | LSB;			//Define 2-bit value read from encoder.
	int sum = (the_encoder->last_code << 2) | code;	//Define 4-bit value:	2 msb is the previous value from the encoder.
							//			2 lsb is the current value from the encoder.

	//State table for determining rotation direction based on previous and current encoder channel values.
	//  +--------Previous reading.
	//  |    +---Current Reading
	//  |    |
	//| AB | AB | sum  | direction	| volume_delta	|
	//| 00 | 00 | 0000 |   none	|      0	|
	//| 00 | 01 | 0001 |   ccw	|     -1	|
	//| 00 | 10 | 0010 |   cw	|      1	|
	//| 00 | 11 | 0011 |   error	|      0	|
	//| 01 | 00 | 0100 |   cw	|      1	|
	//| 01 | 01 | 0101 |   none	|      0	|
	//| 01 | 10 | 0110 |   error	|      0	|
	//| 01 | 11 | 0111 |   ccw	|     -1	|
	//| 10 | 00 | 1000 |   ccw	|     -1	|
	//| 10 | 01 | 1001 |   error	|      0	|
	//| 10 | 10 | 1010 |   none	|      0	|
	//| 10 | 11 | 1011 |   cw	|      1	|
	//| 11 | 00 | 1100 |   error	|      0	|
	//| 11 | 01 | 1101 |   cw	|      1	|
	//| 11 | 10 | 1110 |   ccw	|     -1	|
	//| 11 | 11 | 1111 |   none	|      0	|

	//Progression of 'sum' when turning clockwise:
	//00 -> 10 -> 11 -> 01 -> 00
	//Progression of 'sum' when turning counter-clockwise:
	//00 -> 01 -> 11 -> 10 -> 00

	//Use the following two lines to register every single pulse.
	//That's 4 pulses per detent on ADA377 encoder.
	//if(sum == 0b0010 || sum == 0b1011 || sum == 0b1101 || sum == 0b0100) the_encoder->value++;	//Turned clockwise.
	//if(sum == 0b0001 || sum == 0b0111 || sum == 0b1110 || sum == 0b1000) the_encoder->value--;	//Turned counter-clockwise.

	//Use the following two lines to register only every second pulse.
	//That's equivalent to two pulses for every detent on ADA377 encoder.
	if (sum == 0b0010 || sum == 0b1101) the_encoder->volume_delta++;	//Turned clockwise.
	if (sum == 0b0001 || sum == 0b1110) the_encoder->volume_delta--;	//Turned counter-clockwise.

	the_encoder->last_code = code;	//Update value of last_code for comparison the next time this ISR is triggered.
}


//Interrupt subroutine to be triggered by pressing (or releasing) the toggle button.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick (renamed time).
void button_ISR(int32_t gpio, int32_t level, uint32_t time)
{
	if (gpioRead(the_encoder->button_pin) == LOW) the_encoder->button_pressed_time = time;

	if ((gpioRead(the_encoder->button_pin) == HIGH) && ((time - the_encoder->button_released_time) > DEBOUNCE_US))
	{
		if (the_encoder->button_signal == SIGNAL_STOP)	the_encoder->button_signal = SIGNAL_NULL;	//Clear stop signal.
		else						the_encoder->button_signal = SIGNAL_TOGGLE;	//Set toggle signal.

		the_encoder->button_released_time = time;	//Reset the de-bounce timestamp.
	}
}


//Function returns true if the toggle button has been held down for a specified duration (TOGGLE_BUTTON_LONG_PRESS microseconds).
void poll_long_press(struct encoder *enc)
{
	if ((gpioRead(enc->button_pin) == LOW) && ((gpioTick() - enc->button_pressed_time) > TOGGLE_BUTTON_LONG_PRESS)) 
	{
		enc->button_signal = SIGNAL_STOP;
	}
}


//Triggered in the main loop by polling for a change in the encoder value.
void update_mpd_volume(volatile int8_t *delta_address)
{
	mpd_run_change_volume(connection, *delta_address);	//Change the volume.
	*delta_address = 0;					//Clear the desired volume delta.
}


//Triggered in the main loop by polling the toggle flag being set (by ISR).
void update_mpd_state(volatile uint8_t *signal_address)
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


//Initialise the hardware inputs - two channels on the rotary encoder and a push-button.
bool init_hardware(void)
{
	if (gpioInitialise() == PI_INIT_FAILED) return(FALSE);				//Initialise gpio pins on raspi.

	the_encoder = init_encoder(VOL_ENCODER_A_PIN, VOL_ENCODER_B_PIN, BUTTON_PIN);	//Initialise global struct "the_encoder".
	if (the_encoder == NULL) return(FALSE);						//Return false if initialisation fails.

	return(TRUE);
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

		poll_long_press(the_encoder);							//Poll for button long-press.
		if (the_encoder->volume_delta)	update_mpd_volume(&the_encoder->volume_delta);	//Poll for encoder value change.
		if (the_encoder->button_signal)	update_mpd_state(&the_encoder->button_signal);	//Poll toggle_signal flag.
	}

	return(-1);	//Never reached.
}

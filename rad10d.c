#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		//required for daemonisation
#include <sys/stat.h>		//required for daemonisation

#include <mpd/client.h>				//Required to interface with mpc (mpd client)
#include <wiringPi.h>				//Required for utilising raspberry pi gpio
#include "rotaryencoder/rotaryencoder.h"	//Required to interface a rotary encoder on the raspi gpio

#define vol_encoder_A	15	//Encoder channel A
#define vol_encoder_B	16	//Encoder channel B
#define toggle_pin	1	//Play/pause toggle button
#define debounce_ms	50	//Debounce value for toggle button (in ms)

//#define TOGGLE_FALSE	0	//bool values to indicate whether a toggle button press has registered (toggle_flag)
//#define TOGGLE_TRUE	1
//#define TOGGLE_UNKNOWN	2

//#define PLAY	1
//#define PAUSE	0

//define settings for establishing an mpd connection
#define MPD_SERVER	"127.0.0.1"	//Set the mpd server IP to the localhost for the purpose of this daemon
#define MPD_PORT	6600		//Use the default mpd port - 6600
#define MPD_TIMEOUT	0		//Set the connection timeout as 0 to use the default.

struct mpd_connection *connection = NULL;	//Initialise globally accessible structure to contatin mpd connection info (refer "mpd/client.h")
//function required for initialising the mpd interface via mpc.  this function pulled straight from example code
static struct mpd_connection *setup_connection(void)
{
	struct mpd_connection *conn;

	conn = mpd_connection_new(MPD_SERVER, MPD_PORT, MPD_TIMEOUT);	//settings include IP address of mpd server, port # and connection timeout
	if (conn == NULL) { exit(EXIT_FAILURE);	}		//exit on failure to initialise mpd connection
	return conn;						//return the address of the connection struct
}

//function to return the current mpd status. returns:
//MPD_STATE_UNKNOWN     no information available
//MPD_STATE_STOP        not playing
//MPD_STATE_PLAY        playing
//MPD_STATE_PAUSE       playing, but paused
int get_mpd_status()
{
	struct mpd_status *status_struct = mpd_run_status(connection);	//creates a struct "status_struct" and fills it with info representing the status of the mpd connection "connection"
	int status = mpd_status_get_state(status_struct);		//pulls the unknown/stop/play/pause status from "status_struct"
	mpd_status_free(status_struct);					//releases "status_struct" from the heap
	return status;							//returns the mpd status
}

//Interrupt subroutine to be triggered by pressing the toggle button
void toggleISR ()
{
	delay(debounce_ms);				//wait for a "debounce" duration
	if(digitalRead(toggle_pin) == 0)		//check if the toggle button is still pushed
	{
		if ((get_mpd_status() == MPD_STATE_UNKNOWN) || (get_mpd_status() == MPD_STATE_STOP) || (get_mpd_status() == MPD_STATE_PAUSE)) //first check if status is neither pause nor play
		{
			mpd_run_stop(connection);
			mpd_run_play(connection);
		}
		else							//this is the usual condition after the toggle button is detected
		{
			mpd_run_pause(connection, TRUE);
		}
		while(digitalRead(toggle_pin) == 0) { }			//infinite loop until toggle button is released
	}
}

//main program loop.  arguments (argv[]) and number of arguments (argc) ignored
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

	if (wiringPiSetup() < 0) { exit(EXIT_FAILURE); }	//initialise raspi to interface with gpio, exit if failure

	pinMode (toggle_pin, INPUT);		//set the pin to which the toggle button is connected as an input
	pullUpDnControl (toggle_pin, PUD_UP);	//enable internal pull-up resistor on toggle pin
	if (wiringPiISR (toggle_pin, INT_EDGE_FALLING, &toggleISR) < 0) { exit(EXIT_FAILURE); } //set up interrupt on the toggle button, exit if failure

	the_encoder = init_encoder(vol_encoder_A,vol_encoder_B);	//initialise values stored in globally accessible struct "the_encoder"
	if(the_encoder == NULL) { exit(EXIT_FAILURE); }			//exit if initialisation failed
	int old_encoder_value = the_encoder->value;			//initialise old_encoder_value against which the_encoder->value will be compared to determine if the encoder has moved
	int change = 0;    						//initialise "change" which will be the amount by which the encode value has changed

	connection = setup_connection();        	//initialise mpc connection session
	mpd_connection_set_keepalive(connection, true);	//enable TCP keepalives to prevent disconnection in the event of a long idle duration

	//the main infinite loop
	while (1)
	{
		delay(10);

		if (old_encoder_value != the_encoder->value)		//loop until change detected on the encoder
		{
			change = the_encoder->value - old_encoder_value;        //quantify the amount by which the encode value has changed
			mpd_run_change_volume(connection, change);              //adjust the volume accordingly
			old_encoder_value = the_encoder->value;                 //update known encoder value for next comparison
		}
	
		if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS)	//If an error event has occurred.
		{
			mpd_connection_clear_error(connection);		//attempt to clear any error that may arise
		} 
	}

	return (0);	//never reached
}

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

#define TOGGLE_FALSE	0	//bool values to indicate whether a toggle button press has registered (toggle_flag)
#define TOGGLE_TRUE	1

struct mpd_connection *connection = NULL;	//Initialise globally accessible structure to contatin mpd connection info (refer "mpd/client.h")
//function required for initialising the mpd interface via mpc.  this function pulled straight from example code
static struct mpd_connection *setup_connection(void)
{
	struct mpd_connection *conn;

	conn = mpd_connection_new("127.0.0.1", 6600, 0);	//settings include IP address set to localhost and default mpd port
	if (conn == NULL) { exit(EXIT_FAILURE);	}		//exit on failure to initialise mpd connection
	return conn;						//return the address of the connection struct
}

int toggle_flag = TOGGLE_FALSE;	//initialise global variable that triggers a toggle of mpd pause/play
//Interrupt subroutine to be triggered by pressing the toggle button
void toggleISR ()
{
	delay(debounce_ms);				//wait for a "debounce" duration
	if(digitalRead(toggle_pin) == 0)		//check if the toggle button is still pushed
	{
		toggle_flag = TOGGLE_TRUE;		//set flag that indicates the toggle button press has been registered and mpd requires a play/pause toggle
		while(digitalRead(toggle_pin) == 0) { }	//infinite loop until toggle button is released
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

		if (toggle_flag != TOGGLE_FALSE)		//if the toggle flag has been set by the toggle button isr
		{
			mpd_run_toggle_pause(connection);	//toggle play/pause in mpd
			toggle_flag = TOGGLE_FALSE;		//clear the toggle flag
		}

		mpd_run_change_volume(connection, 0);	//action sets the volume to current value (no change).  Seem to require something in the loop to refresh "connection" and prevent "status=11/SEGV" (syslog)
	}

	return (0);	//never reached
}

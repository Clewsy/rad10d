// rad10d.h - rad10d header file
// for project deatils visit https://gitlab.com/clewsy/rad10d
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//library inclusions
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>		//required for daemonisation.
#include <sys/stat.h>		//required for daemonisation.

#include "libmpdclient-2.16/include/mpd/client.h"	//Required to interface with mpc (mpd client).
#include <wiringPi.h>					//Required for utilising raspberry pi gpio.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//hardware definitions.
#define VOL_ENCODER_A_PIN	15	//Encoder channel A.
#define VOL_ENCODER_B_PIN	16	//Encoder channel B.
#define TOGGLE_PIN		1	//Play/pause toggle button.
#define DEBOUNCE_MS		50	//Debounce value for toggle button (in ms).

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//mpd connection definitions.
#define MPD_SERVER	"127.0.0.1"	//Set the mpd server IP to the localhost for the purpose of this daemon.
#define MPD_PORT	6600		//Use the default mpd port - 6600.
#define MPD_TIMEOUT	1000		//Connection timeout (in ms).  Set to 0 to use the default.

struct mpd_connection *connection = NULL;	//Initialise globally accessible structure to contatin mpd connection info (refer "mpd/client.h").

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//global variable declarations.

//define the structure that represents data from the encoder
struct encoder
{
    int channel_a;			//hardware pin to which encoder A channel is connected.
    int channel_b;			//hardware pin to which encoder B channel is connected.
    volatile long value;		//current value determined by the encoder.
    volatile long last_value;		//last value determined by the encoder.  value & last_value are compared to determine volume delta.
    volatile int last_code;		//last code needed to compare current read from channels a & b to last read.
};

//declare structure named the_encoder - gloablly accessible (used in ISR)
struct encoder *the_encoder;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//function declarations
static struct mpd_connection *setup_connection(void);
bool init_mpd(void);
bool mpd_reconnect(void);
int get_mpd_status(void);
struct encoder *init_encoder(int channel_a, int channel_b);
void updateEncoderISR(void);
void toggleISR(void);
bool init_hardware(void);

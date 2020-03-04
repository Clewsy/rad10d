// rad10d.h - rad10d header file
// For project details visit https://clews.pro/projects/rad10.html and https://gitlab.com/clewsy/rad10d

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//library inclusions.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>		//required for daemonisation.
#include <sys/stat.h>		//required for daemonisation.

#include <mpd/client.h>		//Required to interface with mpd.
#include <pigpio.h>		//Required for utilising raspberry pi gpio.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Boolean definitions.
#define TRUE	1
#define FALSE	0

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Hardware definitions.  Pinout numbering for use with the pigpio is the same as the Broadcom chip numbering (note, != wiringPi numbering).
#define VOL_ENCODER_A_PIN	14	//Encoder channel A.
#define VOL_ENCODER_B_PIN	15	//Encoder channel B.
#define TOGGLE_PIN		18	//Play/pause toggle button.
#define DEBOUNCE_US		50000	//Debounce value for toggle button (in microseconds).

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//mpd connection definitions.
#define MPD_SERVER	"127.0.0.1"	//Set the mpd server IP to the localhost for the purpose of this daemon.
#define MPD_PORT	6600		//Use the default mpd port - 6600.  If the default is not used, this must match "port" in /etc/mpd.conf
#define MPD_TIMEOUT	1000		//Connection timeout (in ms).
#define IDLE_DELAY	10000		//Delay within the main loop to reduce cpu load (in ms).

struct mpd_connection *connection = NULL;	//Initialise globally accessible structure contatining mpd connection info (refer "mpd/client.h").

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Global variable declarations.

//Define the structure that represents data from the encoder.
struct encoder
{
	int channel_a;			//Hardware pin to which encoder A channel is connected.
	int channel_b;			//Hardware pin to which encoder B channel is connected.
	volatile long value;		//Current value determined by the encoder.
	volatile long last_value;	//Last value determined by the encoder.  value & last_value are compared to determine volume delta.
	volatile int last_code;		//Last code needed to compare current read from channels a & b to last read.
};

//Declare structure named the_encoder - gloablly accessible (used in ISR).
struct encoder *the_encoder;

//Declare and initialise the toggleSignal flag - globally accessible (used in ISR).
bool toggleSignal = FALSE;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Function declarations.
static struct mpd_connection *setup_connection(void);
bool init_mpd(void);
bool mpd_reconnect(void);
int get_mpd_status(void);
struct encoder *init_encoder(int channel_a, int channel_b);
void updateEncoderISR(int gpio, int level, uint32_t tick);
void toggleISR(int gpio, int level, uint32_t tick);
bool init_hardware(void);

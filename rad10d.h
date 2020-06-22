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
#define VOL_ENCODER_A_PIN		14	//Encoder channel A.
#define VOL_ENCODER_B_PIN		15	//Encoder channel B.
#define TOGGLE_PIN			18	//Play/pause toggle button.
#define LOW				0	//Pin grounded (e.g. button pressed).
#define HIGH				1	//Pin pulled high.
#define DEBOUNCE_US			100000	//Debounce value for toggle button (in microseconds).
#define TOGGLE_BUTTON_LONG_PRESS	2000000	//Long press duration (in microseconds).

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//mpd connection definitions.
#define MPD_SERVER	"127.0.0.1"	//Set the mpd server IP to the localhost for the purpose of this daemon.
#define MPD_PORT	6600		//Use the default mpd port - 6600.  If the default is not used, this must match "port" in /etc/mpd.conf
#define MPD_TIMEOUT	1000		//Connection timeout (in ms).
#define IDLE_DELAY	10000		//Delay within the main loop to reduce cpu load (in microseconds).

struct mpd_connection *connection = NULL;	//Initialise globally accessible structure contatining mpd connection info (refer "mpd/client.h").

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Global variable declarations.

//Define the structure that represents data from the encoder.
struct encoder
{
	uint8_t channel_a;		//Hardware pin to which encoder A channel is connected.
	uint8_t channel_b;		//Hardware pin to which encoder B channel is connected.
	volatile uint8_t last_code;	//Last code detected by the encoder.  Compare with current code to determine direction of rotation.
	volatile int8_t volume_delta;	//Positive or negative value depending on direction encoder was turned.  Represents desired volume delta.
};

//Declare structure named the_encoder - gloablly accessible (used in ISR).
struct encoder *the_encoder;

//Declare and initialise the variables referenced in the button toggle ISR.
bool toggle_signal = FALSE;		//Flag indicates whether or not a button press has been registered.
bool block_toggle = FALSE;		//Flag indicates a long press of the toggle button, so block toggling when the button is released.
bool toggle_button_held = FALSE;	//Flag indicates whether or not the last loop iteration determined the toggle button was held down. 
uint32_t last_button_trigger = 0;	//Timer value (microseconds) used as a reference to debounce button press.
uint32_t time_when_pressed = 0;		//Initialise the time stamp used to detect a long-press of the toggle button.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Function declarations.
static struct mpd_connection *setup_connection(void);
bool init_mpd(void);
bool mpd_reconnect(void);
uint8_t get_mpd_status(void);
struct encoder *init_encoder(uint8_t channel_a, uint8_t channel_b);
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick);
void toggle_ISR(int32_t gpio, int32_t level, uint32_t tick);
bool toggle_long_press(void);
void update_volume(void);
void update_toggle(void);
bool init_hardware(void);

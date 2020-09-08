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
#define BUTTON_PIN		18	//Play/pause toggle button.
#define LOW			0	//Pin grounded (e.g. button pressed).
#define HIGH			1	//Pin pulled high.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Timer duration definitions.
#define DEBOUNCE_US			100000	//Debounce value for toggle button (in microseconds).
#define TOGGLE_BUTTON_LONG_PRESS	2000000	//Long press duration (in microseconds).

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//mpd control definitions.
#define SIGNAL_NULL	0	//mpd control signal set by the push-button.  SIGNAL_NULL:	No command.
#define SIGNAL_TOGGLE	1	//mpd control signal set by the push-button.  SIGNAL_TOGGLE:	Send the toggle command.
#define SIGNAL_STOP	2	//mpd control signal set by the push-button.  SIGNAL_STOP:	Send the stop command.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//mpd connection definitions.
#define MPD_SERVER	"127.0.0.1"	//Set the mpd server IP to the localhost for the purpose of this daemon.
#define MPD_PORT	6600		//Use the default mpd port - 6600.  If the default is not used, this must match "port" in /etc/mpd.conf
#define MPD_TIMEOUT	1000		//Connection timeout (in ms).
#define IDLE_DELAY	10000		//Delay within the main loop to reduce cpu load (in microseconds).

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Type definitions.

//Define the structure that represents data from the encoder.
struct encoder
{
	uint8_t channel_a;			//Hardware pin to which encoder A channel is connected.
	uint8_t channel_b;			//Hardware pin to which encoder B channel is connected.
	volatile uint8_t last_code;		//Last code detected by the encoder.  Compare with current code to determine direction of rotation.
	volatile int8_t volume_delta;		//Positive or negative depending on direction encoder was turned.  Represents desired volume delta.

	uint8_t button_pin;			//Hardware pin to which the push-buttonis connected.
	volatile uint32_t button_pressed_time;	//Timestamp set whenever a button press is detected and validated (de-bounced).
	volatile uint32_t button_released_time;	//Timestamp set whenever a button release is detected and validated (de-bounced).
	volatile uint8_t button_signal;		//mpd control signal set by the push button.  SIGNAL_NULL, SIGNAL_TOGGLE or SIGNAL_STOP.
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Function declarations.
static struct mpd_connection *setup_connection(void);
bool init_mpd(void);
bool mpd_reconnect(void);
uint8_t get_mpd_status(void);
struct encoder *init_encoder(uint8_t channel_a, uint8_t channel_b, uint8_t toggle_pin);
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick);
void button_ISR(int32_t gpio, int32_t level, uint32_t tick);
void poll_long_press(struct encoder *enc);
void update_mpd_volume(volatile int8_t *delta_adddress);
void update_mpd_state(volatile uint8_t *signal_address);
bool init_hardware(void);

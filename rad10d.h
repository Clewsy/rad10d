// rad10d.h - rad10d header file
// For project details visit:
//  - https://clews.pro/projects/rad10.php and
//  - https://gitlab.com/clewsy/rad10d

////////////////////////////////////////////////////////////////////////////////
//library inclusions.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>     //required for daemonisation.
#include <sys/stat.h>   //required for daemonisation.

//Required to interface with mpd.
//Refer to https://www.musicpd.org/libs/libmpdclient/
#include <mpd/client.h>

//Required for utilising raspberry pi gpio.
//Refer to http://abyz.me.uk/rpi/pigpio/cif.html
#include <pigpio.h>

////////////////////////////////////////////////////////////////////////////////
//Boolean definitions.
#define TRUE    1
#define FALSE   0

////////////////////////////////////////////////////////////////////////////////
//Hardware definitions.  Pinout numbering for use with the pigpio is the same as
// the Broadcom chip numbering (different to wiringPi library numbering).
#define VOL_ENCODER_A_PIN   14  //Encoder channel A.
#define VOL_ENCODER_B_PIN   15  //Encoder channel B.
#define BUTTON_PIN          18  //Play/pause toggle button.
#define LOW                 0   //Pin grounded (i.e. button pressed).
#define HIGH                1   //Pin pulled high (i.e. button released).

////////////////////////////////////////////////////////////////////////////////
//Timer duration definitions.
#define DEBOUNCE_US                 100000  //Debounce for button (in µs).
#define TOGGLE_BUTTON_LONG_PRESS_US 2000000 //Long press duration (in µs).

////////////////////////////////////////////////////////////////////////////////
//mpd control definitions.
#define SIGNAL_NULL     0   //No command.
#define SIGNAL_TOGGLE   1   //Send the toggle command.
#define SIGNAL_STOP     2   //Send the stop command.

////////////////////////////////////////////////////////////////////////////////
//mpd connection definitions.
#define MPD_SERVER      "127.0.0.1" //mpd server IP set to localhost.
#define MPD_PORT        6600        //Using the mpd default port (6600).
#define MPD_TIMEOUT     1000        //Connection timeout (in ms).
#define IDLE_DELAY_US   10000       //Main loop delay to reduce cpu load (ms).

////////////////////////////////////////////////////////////////////////////////
//Define a structure for conveniently storing button press/release timestamps
struct button_timer
{
    uint32_t pressed;   //Timestamp when the button goes low.
    uint32_t released;  //Timestamp when the button goes high.
};

////////////////////////////////////////////////////////////////////////////////
//Function declarations.
static struct mpd_connection *setup_connection(void);
bool init_mpd(void);
bool mpd_reconnect(void);
uint8_t get_mpd_status(void);
bool init_encoder(void);
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick);
void button_ISR(int32_t gpio, int32_t level, uint32_t tick);
void poll_long_press(uint8_t *signal_address);
void update_mpd_state(uint8_t *signal_address);
void update_mpd_volume(int8_t *delta_adddress);
bool init_hardware(void);

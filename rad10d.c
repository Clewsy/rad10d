//rad10d.c - rad10d c code file
//For project details visit:
//  - https://clews.pro/projects/rad10.php and
//  - https://gitlab.com/clewsy/rad10d
//For info regarding the mpd c library (libmpdclient), visit:
//  - https://www.musicpd.org/libs/libmpdclient/.

#include "rad10d.h"


//Global declarations.
int8_t volume_delta = 0;                    //-1, +1 or 0.
uint8_t mpd_control_signal = SIGNAL_NULL;   //Play/pause (toggle) or stop.
struct button_timer button_time = {0, 0};   //Button press/release timestamps.
struct mpd_connection *connection = NULL;   //Contains mpd connection data.


//Create an mpd "connection" structure and return the address of the struct.
static struct mpd_connection *setup_connection(void) {

    struct mpd_connection *conn;    //Initialise the struct.

    conn = mpd_connection_new(MPD_SERVER, MPD_PORT, MPD_TIMEOUT);
    if (conn == NULL) exit(EXIT_FAILURE);   //Exit on failure to initialise.

    return(conn);   //Return the address of the connection struct.
}


//Initialise the mpd connection.  Note: "connection" is a pointer to a struct.
bool init_mpd(void) {

    connection = setup_connection();        //Initialise mpd connection session.
    if(connection == NULL) return(FALSE);
    mpd_connection_set_keepalive(connection, TRUE);    //Enable TCP keepalives.

    return(TRUE);   //Connection has been successfully established.
}


//Attempt to close then restablish an mpd connection.
bool mpd_reconnect(void) {

    mpd_connection_free(connection);    //Close the connection and free memory.
    if (init_mpd() == FALSE) return(FALSE); //Attempt a new mpd connection.

    return(TRUE);   //Connection should be successfully re-established.
}


//Return the current mpd status. Possible return values:
//MPD_STATE_UNKNOWN     = 0    no information available
//MPD_STATE_STOP        = 1    not playing
//MPD_STATE_PLAY        = 2    playing
//MPD_STATE_PAUSE       = 3    playing, but paused
uint8_t get_mpd_status(void) {

    //Create "status_struct" (contains status of "connection").
    struct mpd_status *status_struct = mpd_run_status(connection);
    int status = mpd_status_get_state(status_struct);

    //While the status is anything but STOP, PLAY or PAUSE...
    while ((status < MPD_STATE_STOP) || (status > MPD_STATE_PAUSE))
    {
        while (mpd_reconnect() == FALSE) {};
        status = mpd_status_get_state(status_struct);
    }
    mpd_status_free(status_struct); //Release "status_struct" from the heap.

    return(status); //Return the mpd status.
}


//Function to initialise the struct that reflects the state of the encoder
//hardware.  Returns address of the initialised struct.
bool init_encoder(void) {

    //Set pins as inputs (encoder pins and the button).
    if (gpioSetMode(VOL_ENCODER_A_PIN, PI_INPUT)) return(FALSE);
    if (gpioSetMode(VOL_ENCODER_B_PIN, PI_INPUT)) return(FALSE);
    if (gpioSetMode(BUTTON_PIN, PI_INPUT)) return(FALSE);

    //Enable pull-ups on the input pins.
    if (gpioSetPullUpDown(VOL_ENCODER_A_PIN, PI_PUD_UP)) return(FALSE);
    if (gpioSetPullUpDown(VOL_ENCODER_B_PIN, PI_PUD_UP)) return(FALSE);
    if (gpioSetPullUpDown(BUTTON_PIN, PI_PUD_UP)) return(FALSE);

    //Attach interrupt subroutines to be triggered by state change of inputs.
    if (gpioSetISRFunc(VOL_ENCODER_A_PIN, EITHER_EDGE, 100, &volume_ISR)) return(FALSE);
    if (gpioSetISRFunc(VOL_ENCODER_B_PIN, EITHER_EDGE, 100, &volume_ISR)) return(FALSE);
    if (gpioSetISRFunc(BUTTON_PIN, EITHER_EDGE, 0, &button_ISR)) return(FALSE);

    return(TRUE);    //Initialise was successful.
}


//Interrupt sub-routine that is triggered by any change on either encoder pin.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick.
//These values are ignored in this use-case.
void volume_ISR(int32_t gpio, int32_t level, uint32_t tick)
{
    static uint8_t last_code = 0;   //Static to retain when ISR is triggered,

    //Read in the state of the encoder pins (most- and least-significant bits).
    bool msb = gpioRead(VOL_ENCODER_A_PIN);
    bool lsb = gpioRead(VOL_ENCODER_B_PIN);

    //Combine msb and lsb into 2-bit value.
    uint8_t code = (msb << 1) | lsb;

    //Combine code with last_code to create a 4-bit value to look up in table.
    uint8_t abab = (last_code << 2) | code;

    //State table for determining rotation direction.
    //Determined by comparing previous and current encoder channel values.
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
    //if(abab == 0b0010 || abab == 0b1011 || abab == 0b1101 || abab == 0b0100) volume_delta++;
    //if(abab == 0b0001 || abab == 0b0111 || abab == 0b1110 || abab == 0b1000) volume_delta--;

    //Use the following two lines to register only every second pulse.
    //That's equivalent to two pulses for every detent on ADA377 encoder.
    if (abab == 0b0010 || abab == 0b1101) volume_delta++;   //Clockwise.
    if (abab == 0b0001 || abab == 0b1110) volume_delta--;   //Counter-clockwise.

    last_code = code;    //Update last_code for the next time ISR is triggered.
}


//Interrupt subroutine to be triggered by pressing (or releasing) toggle button.
//The gpioSetISRFunc requires ISRs such as this to receive gpio, level and tick
//(renamed time). Note, action is only taken when the button is released.
void button_ISR(int32_t gpio, int32_t level, uint32_t time) {

    //If button pressed, record the current timestamp as button_time.pressed
    //(needed when polling for long-press).
    if (gpioRead(BUTTON_PIN) == LOW) button_time.pressed = time;

    //If button is not pressed and a time greater than the debounce has elapsed
    //since the last debounced button release. I.e. first "bounce" registered as
    //the release, subsequent bounces within DEBOUNCE period are ignored.
    if ((gpioRead(BUTTON_PIN) == HIGH) && ((time - button_time.released) > DEBOUNCE_US))
    {
        //Legitimate button press detected (debounced).
        if (mpd_control_signal == SIGNAL_STOP)  mpd_control_signal = SIGNAL_NULL;
        else                                    mpd_control_signal = SIGNAL_TOGGLE;

        button_time.released = time;    //Reset the button release timestamp.
    }
}


//Function returns true if the toggle button has been held down for a specified
//duration (TOGGLE_BUTTON_LONG_PRESS_US microseconds).
void poll_long_press(uint8_t *signal_address) {

    //If the button is pressed and has remained pressed for a duration greater
    //than the period that defines a "long press".
    if    ((gpioRead(BUTTON_PIN) == LOW) && ((gpioTick() - button_time.pressed) > TOGGLE_BUTTON_LONG_PRESS_US)) 
    {
        *signal_address = SIGNAL_STOP; //Cleared by button_ISR.
    }
}


//Triggered in the main loop by polling the toggle flag (set by ISR).
void update_mpd_state(uint8_t *signal_address) {
    
    switch(*signal_address)
    {
        case SIGNAL_TOGGLE:
            if (get_mpd_status() == MPD_STATE_PLAY) mpd_run_pause(connection, TRUE);
            else                                    mpd_run_play(connection);
            *signal_address = SIGNAL_NULL;
            break;
        case SIGNAL_STOP:
            mpd_run_stop(connection);
            break;  //Do not reset signal, used as a check in the button ISR.
    }
}


//Triggered in the main loop by polling for a change in the encoder value.
void update_mpd_volume(int8_t *delta_address) {

    mpd_run_change_volume(connection, *delta_address);
    *delta_address = 0; //Clear the desired volume delta.
}


//Initialise the hardware inputs.
bool init_hardware(void)
{
    if (gpioInitialise() == PI_INIT_FAILED) return(FALSE);
    if (!init_encoder()) return(FALSE);

    return(TRUE);   //Successfully initialised.
}


//Main program loop.  arguments (argv[]) and number of arguments (argc) ignored.
int main(int argc, char *argv[])
{
    ////////////////////////////////////////////////////////////////////////////
    ////////    This initial code in main required for daemonisation    ////////
    ////////////////////////////////////////////////////////////////////////////

    pid_t pid = 0;  //Initialise pid (process id).
    pid_t sid = 0;  //Initialise sid (session id).

    //Fork from the parent process.  fork() should return PID of child.
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    //Unmask the file mode (allow daemon to open, read and write any file).
    umask(0);

    //Set new session and allocate session id.  
    sid = setsid();
    
    //If successful, child process is now process group leader.
    if (sid < 0) exit(EXIT_FAILURE);

    //Change working directory. If can't find the directory, exit with failure.
    if ((chdir("/")) < 0) exit(EXIT_FAILURE);

    //Daemons should not involve any standard user interaction.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    ////////////////////////////////////////////////////////////////////////////
    ////////           The Payload (now running as a daemon)            ////////
    ////////////////////////////////////////////////////////////////////////////

    if (!init_hardware()) exit(EXIT_FAILURE);
    if (!init_mpd()) exit(EXIT_FAILURE);

    //The main infinite loop.
    while (TRUE)
    {
        get_mpd_status();           //Loop until valid connection state.
        gpioDelay(IDLE_DELAY_US);   //Don't want to max out cpu usage.

        //Poll for conditions that trigger play/pause, stop of volume control.
        poll_long_press(&mpd_control_signal);
        if (volume_delta)    update_mpd_volume(&volume_delta);
        if (mpd_control_signal)    update_mpd_state(&mpd_control_signal);
    }

    return(-1);    //Never reached.
}

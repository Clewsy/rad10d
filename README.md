# rad10d
A daemon written for an internet radio project using a raspberry pi.  The daemon interfaces with the mpc (mpd client) api to allow volume and play/pause toggle control via a rotary encoder with push-button.  Roatating the encoder adjusts the volume, the push-button toggles play/pause.  Simple!  

My hardware implementation includes a Raspberry Pi 3 with a small amplifier board connected to the 3.5mm audio jack.  The amplifier is set up with a mono output to a single speaker.  

The rotary encoder channels A and B are connected to the Pi's GPIO pins 15 and 16 respectively (physical pins 8 & 10).  

The push-button output is connected to GPIO pin 1 (physical pin 12).

# Setup instructions
Install the dependencies.  Standard raspbian comes with wiringpi but it needs to be installed onto rasbian lite.
```
$ sudo apt update
$ sudo install wiringpi mpd libmpdclient-dev mpc
```
Compile the executable "rad10d":
(Ensure the libmpdclient library is also copied into the source directory - [libmpdclient download page](https://musicpd.org/libs/libmpdclient/)).
```
$ make all
```
Copy files to system directories:
```
$ sudo cp rad10d /usr/local/sbin/rad10d
$ sudo cp rad10d.service /lib/systemd/system/rad10d.service
```
Enable and start the service:  
(This service is created so that the daemon runs at boot).
```
$ sudo systemctl enable rad10d.service
$ sudo systemctl start rad10d.service
```
# Credits
Guidance for developing the rotary encoder interface came from the work done by Andrew Stine.  
https://github.com/astine/rotaryencoder/blob/master/rotaryencoder.c

Daemon uses the MPD client c library.  
https://www.musicpd.org/doc/libmpdclient/index.html

Hardware interfacing with the Raspberry Pi uses the Wiring Pi library by Drogon (Gordon Henderson).  
http://wiringpi.com/

# Photos
Here's my internet rad10!

![rad10 Front View,](photos/rad10_front.jpg)

![rad10 Back View,](photos/rad10_back.jpg)

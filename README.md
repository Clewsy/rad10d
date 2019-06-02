# rad10d
A daemon written for an internet radio project using a raspberry pi.  The daemon interfaces with the mpc (mpd client) api to allow volume and play/pause toggle control via a rotary encoder with push-button.  Roatating the encoder adjusts the volume, the push-button toggles play/pause.  Simple!

# Setup instructions
```Compile the executable "rad10d"  
$make all
```
```Copy files to system directories:  
$sudo cp rad10d /usr/local/sbin/rad10d  
$sudo cp rad10d.service /lib/systemd/system/rad10d.service
```
```Enable and start the service  
$sudo systemctl enable rad10d.service  
$sudo systemctl start rad10d.service
```
# Credits
Developing the rotary encoder interfacing started with the work done by Andrew Stine.  
https://github.com/astine/rotaryencoder/blob/master/rotaryencoder.c

Daemon uses the MPD API library  
https://www.musicpd.org/doc/libmpdclient/index.html

Hardware interfacing with the Raspberry Pi uses the Wiring Pi library by Drogon.  
http://wiringpi.com/

# Photo
Here's my internet rad10!

![rad10 Front View,](photos/rad10_front.jpg)

![rad10 Back View,](photos/rad10_back.jpg)

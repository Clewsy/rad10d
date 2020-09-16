# rad10d
A daemon written for an internet radio project using a [Raspberry Pi][link_web_raspberry_pi].  The daemon interfaces with the [mpd][link_web_mpd] client api to allow **volume** and **play/pause** toggle control via a rotary encoder with ia push-button.  Rotating the encoder adjusts the volume, the push-button toggles play/pause.  Simple!

At least that's how it started.  Eventually I added the ability to **stop** by pressing and holding the toggle button for a couple of seconds.  This became useful for when I want to restart a track or if a stream drops out.

My hardware implementation includes a Raspberry Pi 4 with a small amplifier board connected to the 3.5mm audio jack.  The amplifier is set up with a mono output to a single speaker.  Initially I found (via dmesg) that the raspberry pi occassionally triggered an undervolt warning.  It never crashed, but with some experimenting I found the under-voltage would occassionaly be triggered at higher volumes when the amplifier drew higher currents.  I fixed this by switching to a 4amp  power supply capable of handling the peak loads.

The rotary encoder channels A and B are connected to the Pi's GPIO pins 14 and 15 (Broadcom numbering) respectively (physical pins 8 & 10).  

The push-button is connected to GPIO pin 18 (Broadcom numbering) (physical pin 12).  

Although it is independent to the daemon, this repo also includes a simple web interface for controling mpd.

Additional project detail and photos can be found [here][link_clews_projects_rad10].

## Setup instructions
Install the dependencies.
```shell
$ sudo apt update
$ sudo apt install pigpio mpd mpc libmpdclient-dev
```
Notes:
* Instead of using the package manager to install the mpdclient library (libmpdclient-dev), you can obtain it directly from the [libmpdclient download page][link_web_libmpdclient_download].
* You don't neccessarily need to install [mpc][link_web_mpc] for this project, but it's a great tool for controlling mpd from the command line and also useful for debugging.  It is however required if you wish to use the WebUI.

Clone this repo, compile the executable (**rad10d**) and install:
```shell
$ git clone https://gitlab.com/clewsy/rad10d
$ cd rad10d
$ sudo make install
```
The `sudo make install` command performs the following actions:
1. Compile the rad10d executable file.
2. Copies the compiled executable to `/usr/local/sbin/rad10`.  Ownership is set to root and permissions mode set to 0755.
3. Copies the systemd unit file to `/lib/systemd/system/rad10d.service`.  Ownership is set to root and permissions mode set to 0755.
4. Enables the systemd service so that the daemon is executed at boot.
5. Starts the sytemd service which in turn starts the rad10 daemon.

Instead of running `sudo make install`, these installation tasks could be completed manually with the following commands:
```shell
$ sudo cp rad10d /usr/local/sbin/rad10d
$ sudo cp rad10d.service /lib/systemd/system/rad10d.service
$ sudo systemctl enable rad10d.service
$ sudo systemctl start rad10d.service
```

Note, the daemon can be uninstalled with `sudo make uninstall` which will reverse the installation procedure.

## WebUI
Although not required for the hardware functionality enabled by the instructions above, this repository also includes a simple web-based user interface (WebUI) for similar simple control of mpd on the raspberry pi.  

The interface is written in php/html/css and just gives a play/pause toggle button and volume up/down control.  

It also includes a few internet streaming "presets" that I often use.  Check out the [Icecast Directory][link_web_icecast_directory] to find urls for many streams.

To install the WebUI, the raspberry pi will require installation of php and web server software such as [Apache][link_web_apache], [NGinX][link_web_nginx], [Lighttpd][link_web_lighttpd] or similar.  The following instructions will use Apache.

```shell
$ sudo apt update
$ sudo apt install apache2 php
```
By default, Apache will serve the html/css/php files stored in the /var/www/html/ directory.

From within this directory, delete any existing files then copy the webui files from the previously cloned repository:

```shell
$ cd /var/www/html
$ sudo rm -r *
$ sudo cp --recursive ~/rad10/webui/* .	## Change to suit wherever you cloned the rad10 repository. 
```
The web interface should now be available over your local network.  If the hostname of your raspberry pi remains as the default, then the rad10 WebUI should be accessible from your browser at http://raspberrypi/

## Ansible role
For another project ([clewsy_ansible][link_gitlab_clewsy_clewsy_ansible]) I have been automating deployment, configuration and maintenance of hosts on my home network.  In doing so, I created a couple of [ansible][link_web_ansible] roles that when run, will automatically install the dependencies, download this repo, compile/install the daemon and set up the WebUI.

|Role							|Description																																					|
|-------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|[mpd][link_gitlab_clewsy_clewsy_ansible_roles_mpd]	|This role will install packages mpd, mpc and ncmpc, then configure and run the mpd daemon.  It also creates some handy aliases.																										|
|[rad10][link_gitlab_clewsy_clewsy_ansible_roles_rad10]	|The mpd role is a pre-requisite for this role.  The rad10 role will first run the mpd role, then clone rad10d repo, compile the daemon and configure a unit-file for auto-starting.  It will then install web server packages ([Apache][link_web_apache]) and copy the html/php files for the rad10 webui.	|

## Credits
A starting point for developing the rotary encoder interface daemon came from the [work done by Andrew Stine][link_web_andrew_stine].  

Daemon uses the [MPD client c library][link_web_libmpdclient_library].  

~~Hardware interfacing with the Raspberry Pi uses the [WiringPi library by Drogon (Gordon Henderson)][link_web_wiringpi].~~  

As of August 2019, the wiringPi library has been [deprecated][link_web_wiringpi_deprecated] so I transitioned to the [pigpio library by joan2937][link_web_pigpio].  

## Photos
Here's my internet rad10!

![rad10 Front View][image_rad10_front]

![rad10 Back View][image_rad10_back]

Some screenshots of the WebUI on an android smartphone:

![rad10 WebUI][image_rad10_webui_1]

![rad10 WebUI][image_rad10_webui_2]

[link_clews_projects_rad10]:https://clews.pro/projects/rad10.php

[link_gitlab_clewsy_clewsy_ansible]:https://gitlab.com/clewsy/clewsy_ansible
[link_gitlab_clewsy_clewsy_ansible_roles_mpd]:https://gitlab.com/clewsy/clewsy_ansible/-/tree/master/roles/mpd
[link_gitlab_clewsy_clewsy_ansible_roles_rad10]:https://gitlab.com/clewsy/clewsy_ansible/-/tree/master/roles/rad10

[link_web_andrew_stine]:https://github.com/astine/rotaryencoder/blob/master/rotaryencoder.c
[link_web_ansible]:https://docs.ansible.com/
[link_web_apache]:https://httpd.apache.org/
[link_web_icecast_directory]:https://dir.xiph.org/
[link_web_libmpdclient_download]:https://musicpd.org/libs/libmpdclient/
[link_web_libmpdclient_library]:https://www.musicpd.org/doc/libmpdclient/index.html
[link_web_lighttpd]:https://www.lighttpd.net/
[link_web_mpc]:https://musicpd.org/clients/mpc/
[link_web_mpd]:https://musicpd.org/
[link_web_nginx]:https://nginx.org/
[link_web_pigpio]:http://abyz.me.uk/rpi/pigpio/index.html
[link_web_raspberry_pi]:https://www.raspberrypi.org/
[link_web_wiringpi]:http://wiringpi.com/
[link_web_wiringpi_deprecated]:http://wiringpi.com/wiringpi-deprecated/

[image_rad10_back]:/images/rad10_back.jpg
[image_rad10_front]:/images/rad10_front.jpg
[image_rad10_webui_1]:/images/rad10_webui_1.png
[image_rad10_webui_2]:/images/rad10_webui_2.png

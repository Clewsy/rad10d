#!/bin/bash

#This script will append new alias entries to the ~/.bash_aliases file.
#These entries will provide shortcut commands for initiating audio streams with mpc/mpd.
#Format for each line in the STREAM_ALIASES variable should be:
#	[alias_command] [url_of_stream]
#There should be no spaces in either alias_command or url_of_stream.
#once run, the aliases won't take effect until the user starts a new session or alternatively runs the command: source ~/.bash_aliases

STREAM_ALIASES="\
stream_triplej http://live-radio01.mediahubaustralia.com/2TJW/aac/
stream_coderadio https://coderadio-relay.freecodecamp.org/radio/8010/radio.mp3
stream_nightridefm https://nightride.fm/stream/nightride.m4a
stream_proton http://www.protonradio.com:8000/schedule"

ALIASES_FILE="${HOME}/.bash_aliases"

while read -r STREAM
do
	STREAM_NAME=$(echo ${STREAM} | cut -d ' ' -f 1)
	STREAM_URL=$(echo ${STREAM} | cut -d ' ' -f 2)
	if grep "${STREAM_NAME}" ${ALIASES_FILE} >> /dev/null; then
		echo "Alias for \"${STREAM_NAME}\" already exists, skipping."
	else
		echo "Adding alias for \"${STREAM_NAME}\"."
		echo -e "alias ${STREAM_NAME}=\"mpc clear && mpc add ${STREAM_URL} && mpc play\"" >> ${ALIASES_FILE}
	fi
done <<< ${STREAM_ALIASES}

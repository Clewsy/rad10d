//this code is a greatly modified version of rotaryencoder.c created by Andrew Stine (astine, https://github.com/astine/rotaryencoder/blob/master/rotaryencoder.c)
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "rotaryencoder.h"

//interrupt sub-routine that is triggered by any change on the encoder pins
void updateEncoderISR()
{
	int MSB = digitalRead(the_encoder->pin_a);		//define most-significant bit from 2-bit encoder
	int LSB = digitalRead(the_encoder->pin_b);		//define least-significant bit from 2-bit encoder

	int encoded = (MSB << 1) | LSB;				//define 2-bit value read from encoder
	int sum = (the_encoder->lastEncoded << 2) | encoded;	//define 4-bit value: where 2 msb is the previous value from the encoder and 2 lsb is the current

	//Use the following two lines are required to register every single pulse.
	//That's 4 pulses per detent on ADA377 encoder.
	//if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) the_encoder->value++;
	//if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) the_encoder->value--;

	//Use the following two lines to register only every fourth pulse.
	//That's equivalent to a single pulse for every detend on ADA377 encoder.
	if (sum == 0b0100 || sum == 0b1011) the_encoder->value++;
	if (sum == 0b0111 || sum == 0b1000) the_encoder->value--;

	the_encoder->lastEncoded = encoded;	//update value of lastEncoded for use next time this ISR is triggered
}

//routine to initialise the struct that reflects the state of the encoder
struct encoder *init_encoder(int pin_a, int pin_b)
{
	the_encoder = malloc(sizeof(struct encoder));	//allocates heap memory for globally accesible "the_encoder"

	the_encoder->pin_a = pin_a;			//defines the channel a pin
	the_encoder->pin_b = pin_b;			//defines the channel b pin
	the_encoder->value = 0;				//initialises the encoder value
	the_encoder->lastEncoded = 0;			//initialises lastEncoded value (to recalculate value)

	pinMode(pin_a, INPUT);					//set encoder channel a pin as an input
	pinMode(pin_b, INPUT);					//set encoder channel b pin as an input	
	pullUpDnControl(pin_a, PUD_UP);				//enable pull-up resistor on channel a pin
	pullUpDnControl(pin_b, PUD_UP);				//enable pull-up resistor on channel b pin
	wiringPiISR(pin_a,INT_EDGE_BOTH, updateEncoderISR);	//enable hardware interrupt on pin a and define routine to run
	wiringPiISR(pin_b,INT_EDGE_BOTH, updateEncoderISR);	//enable hardware interrupt on pin b and define routine to run

	return the_encoder;	//returns address of "*the_encoder"
}

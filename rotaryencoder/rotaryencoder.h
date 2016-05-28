//this code is a greatly modified version of rotaryencoder.c created by Andrew Stine (astine, https://github.com/astine/rotaryencoder/blob/master/rotaryencoder.h)

//define the struct structure that represents data from the encoder
struct encoder
{
    int pin_a;
    int pin_b;
    volatile long value;
    volatile int lastEncoded;
};

//Define structure named the_encoder - gloablly accessible (used in ISR)
struct encoder *the_encoder;

//declare function that initialises values in "the_encoder"
struct encoder *init_encoder(int pin_a, int pin_b);

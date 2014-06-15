/*
This code drives two sets of dual 595 shift registers. One set controls the 'power cell' LEDs of
the proton pack, the other runs the 'bargraph' and two firing LEDs. Cyclotron lights are controlled
by a 4017 decade counter and 555 timer.

This code is based on code by Jeremey Choi, amendments have been made to improve clarity.
Thanks to Jeremy who did almost all the work and freely shared the code.

N.B. I'm not going for screen accuracy, simply something that looks pretty close. If you want to use
the code in your own project the only thing I ask is that you share any modifications.

Additions: On setup the powercell is reset (previously it may have had random lights on)
On poweroff there is a 'rewind' on the powercell lights and an alternative poweroff sound.
If you don't like it comment it out. :)
*/


// Required by SD card
#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"

SdReader card;    // This object holds the information for the card.
FatVolume vol;    // This holds the information for the partition on the card.
FatReader root;   // This holds the information for the filesystem on the card.
FatReader f;      // This holds the information for the file we're play.
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time.

// Timekeeping
long previousMillis_off = 0;  // will store last updated time.
long previousMillis_on = 0;
long previousMillis_fire = 0;
long off_interval = 30;
long pwrcl_interval = 60;     // interval at which to cycle lights (milliseconds).
long firing_interval = 40;    // interval at which to cycle firing lights (milliseconds).

// ===============================================================================================
// BUTTONS
// array is in the format: buttons[power_on, gun_fire, music_change]
// thus, whatever pin is set to buttons[0] switches on the power cell.
// ===============================================================================================
#define DEBOUNCE 5  // button debouncer.
byte buttons[] = {14, 15, 16};  // Analog 0, 1 and 2 respectively.
// This handy macro lets us determine how big the array up above is, by checking the size.
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'pressed' (the current state).
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

//Pin connected to ST_CP of 74HC595 (pin 12)
int latchPin = 8;
//Pin connected to SH_CP of 74HC595 (pin 11)
int clockPinPWR = 6;  // Power cell
int clockPinGUN = 18;  // Gun
////Pin connected to DS of 74HC595 (pin 14)
int dataPinPWR = 7;  // Power cell
int dataPinGUN = 19; // Gun

// PINS SUMMARY:
// 2, 3, 4, 5, 10 = audioshield
// 6, 7, 18, 19 = 595's
// 11, 12, 13 = SD card i/o

//holders for information we're going to pass to shifting function
byte dataPWRCL1;
byte dataPWRCL2;
byte dataGUN1;
byte dataGUN2;
byte dataGUNON1;
byte dataGUNON2;
// data arrays
byte daPWRCL1[15];
byte daPWRCL2[15];
byte daGUN1[26];
byte daGUN2[26];
byte daGUNON1[15];
byte daGUNON2[15];
   
int freeRam(void) {
    extern int  __bss_end;
    extern int  *__brkval;
    int free_memory;
    if((int)__brkval == 0) {
        free_memory = ((int)&free_memory) - ((int)&__bss_end); 
    }
    else {
        free_memory = ((int)&free_memory) - ((int)__brkval); 
    }
    return free_memory; 
}


void setup() {
    byte i;
    
    /*
    // set up serial port
    Serial.begin(9600);
    putstring_nl("WaveHC with ");
    Serial.print(NUMBUTTONS, DEC);
    putstring_nl("buttons");
    
    putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
    Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!
    */
    
    // Set the output pins for the DAC control. This pins are defined in the library
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(10, OUTPUT);
  
    pinMode(latchPin, OUTPUT);
    // power cell LEDs 0 through 7
    daPWRCL1[0] = 0x80;  //10000000
    daPWRCL1[1] = 0xC0;  //11000000
    daPWRCL1[2] = 0xE0;  //11100000
    daPWRCL1[3] = 0xF0;  //11110000
    daPWRCL1[4] = 0xF8;  //11111000
    daPWRCL1[5] = 0xFC;  //11111100
    daPWRCL1[6] = 0xFE;  //11111110
    daPWRCL1[7] = 0xFF;  //11111111
    daPWRCL1[8] = 0xFF;  //11111111
    daPWRCL1[9] = 0xFF;  //11111111
    daPWRCL1[10] = 0xFF; //11111111
    daPWRCL1[11] = 0xFF; //11111111
    daPWRCL1[12] = 0xFF; //11111111
    daPWRCL1[13] = 0xFF; //11111111
    daPWRCL1[14] = 0xFF; //11111111
    
    // Power cell LEDs 8 through 15
    daPWRCL2[0] = 0x00;  //00000000
    daPWRCL2[1] = 0x00;  //00000000
    daPWRCL2[2] = 0x00;  //00000000
    daPWRCL2[3] = 0x00;  //00000000
    daPWRCL2[4] = 0x00;  //00000000
    daPWRCL2[5] = 0x00;  //00000000
    daPWRCL2[6] = 0x00;  //00000000
    daPWRCL2[7] = 0x80;  //10000000
    daPWRCL2[8] = 0xC0;  //11000000
    daPWRCL2[9] = 0xE0;  //11100000
    daPWRCL2[10] = 0xF0; //11110000
    daPWRCL2[11] = 0xF8; //11111000
    daPWRCL2[12] = 0xFC; //11111100
    daPWRCL2[13] = 0xFE; //11111110
    daPWRCL2[14] = 0xFF; //11111111
    
    //second lights
    daGUN1[0] = 0x00;  //00000000
    daGUN1[1] = 0x00;  //00000000
    daGUN1[2] = 0x00;  //00000000
    daGUN1[3] = 0x00;  //00000000
    daGUN1[4] = 0x00;  //00000000
    daGUN1[5] = 0x00;  //00000000
    daGUN1[6] = 0x00;  //00000000
    daGUN1[7] = 0x80;  //10000000
    daGUN1[8] = 0xC0;  //11000000
    daGUN1[9] = 0xE0;  //11100000
    daGUN1[10] = 0xF0; //11110000
    daGUN1[11] = 0xF8; //11111000
    daGUN1[12] = 0xFC; //11111100
    daGUN1[13] = 0xFE; //11111110
    daGUN1[14] = 0xFC; //11111100
    daGUN1[15] = 0xF8; //11111000
    daGUN1[16] = 0xF0; //11110000
    daGUN1[17] = 0xE0; //11100000
    daGUN1[18] = 0xC0; //11000000
    daGUN1[19] = 0x80; //10000000
    daGUN1[20] = 0x00; //00000000
    daGUN1[21] = 0x00; //00000000
    daGUN1[22] = 0x00; //00000000
    daGUN1[23] = 0x00; //00000000
    daGUN1[24] = 0x00; //00000000
    daGUN1[25] = 0x00; //00000000
    daGUN1[26] = 0x00; //00000000
    
    //first lights
    daGUN2[0] = 0x40;  //01000000
    daGUN2[1] = 0x40;  //01000000
    daGUN2[2] = 0x60;  //01100000
    daGUN2[3] = 0x70;  //01110000
    daGUN2[4] = 0x78;  //01111000
    daGUN2[5] = 0x7C;  //01111100
    daGUN2[6] = 0x7E;  //01111110
    daGUN2[7] = 0x7F;  //01111111
    daGUN2[8] = 0x7F;  //01111111
    daGUN2[9] = 0x7F;  //01111111
    daGUN2[10] = 0x7F; //01111111
    daGUN2[11] = 0x7F; //01111111
    daGUN2[12] = 0x7F; //01111111
    daGUN2[13] = 0x7F; //01111111
    daGUN2[14] = 0x7F; //01111111
    daGUN2[15] = 0x7F; //01111111
    daGUN2[16] = 0x7F; //01111111
    daGUN2[17] = 0x7F; //01111111
    daGUN2[18] = 0x7F; //01111111
    daGUN2[19] = 0x7F; //01111111
    daGUN2[20] = 0x7F; //01111111
    daGUN2[21] = 0x7E; //01111110
    daGUN2[22] = 0x7C; //01111100
    daGUN2[23] = 0x78; //01111000
    daGUN2[24] = 0x70; //01110000
    daGUN2[25] = 0x60; //01100000
    daGUN2[26] = 0x40; //01000000
    
    // 7 led array LEFT
    // 10000000 used for GUN strobe
    daGUNON2[0] = 0xC0;  //11000000
    daGUNON2[1] = 0x20;  //00100000
    daGUNON2[2] = 0x10;  //00010000
    daGUNON2[3] = 0x88;  //10001000
    daGUNON2[4] = 0x04;  //00000100
    daGUNON2[5] = 0x02;  //00000010
    daGUNON2[6] = 0x81;  //10000001
    daGUNON2[7] = 0x01;  //00000001
    daGUNON2[8] = 0x81;  //10000001
    daGUNON2[9] = 0x02;  //00000010
    daGUNON2[10] = 0x84; //10000100
    daGUNON2[11] = 0x08; //00001000
    daGUNON2[12] = 0x10; //00010000
    daGUNON2[13] = 0xA0; //10100000
    daGUNON2[14] = 0x40; //01000000
    
    // 7 led array
    // 00000001 used for gun strobe
    daGUNON1[0] = 0x02;  //00000010
    daGUNON1[1] = 0x05;  //00000101
    daGUNON1[2] = 0x08;  //00001000
    daGUNON1[3] = 0x11;  //00010001
    daGUNON1[4] = 0x20;  //00100000
    daGUNON1[5] = 0x40;  //01000000
    daGUNON1[6] = 0x81;  //10000001
    daGUNON1[7] = 0x80;  //10000000
    daGUNON1[8] = 0x81;  //10000001
    daGUNON1[9] = 0x40;  //01000000
    daGUNON1[10] = 0x20; //00100000
    daGUNON1[11] = 0x11; //00010001
    daGUNON1[12] = 0x08; //00001000
    daGUNON1[13] = 0x05; //00000101
    daGUNON1[14] = 0x02; //00000010
  
  
    // Make input & enable pull-up resistors on switch pins
    for (i=0; i< NUMBUTTONS; i++) {
        pinMode(buttons[i], INPUT);
        digitalWrite(buttons[i], HIGH);
    }
    
    // Initialise Power cell and gun lights
    digitalWrite(latchPin, 0);
    shiftOut(dataPinPWR, clockPinPWR, 0x00);
    shiftOut(dataPinPWR, clockPinPWR, 0x00);
    shiftOut(dataPinGUN, clockPinGUN, 0x40);
    shiftOut(dataPinGUN, clockPinGUN, 0x00);
    digitalWrite(latchPin, 1);
    
    
    //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
    if (!card.init()) {         //play with 8 MHz spi (default faster!)  
        putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
        sdErrorCheck();
        while(1);                            // then 'halt' - do nothing!
    }
    
    // enable optimize read - some cards may timeout. Disable if you're having problems
    card.partialBlockRead(true);
   
    // Now we will look for a FAT partition!
    uint8_t part;
    for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
        if (vol.init(card, part)) 
            break;                         // we found one, lets bail
    }
    if (part == 5) {                       // if we ended up not finding one  :(
        putstring_nl("No valid FAT partition!");
        sdErrorCheck();      // Something went wrong, lets print out why
        while(1);            // then 'halt' - do nothing!
    }
    
    /*
    // Lets tell the user about what we found
    putstring("Using partition ");
    Serial.print(part, DEC);
    putstring(", type is FAT");
    Serial.println(vol.fatType(), DEC);     // FAT16 or FAT32?
    */
    
    // Try to open the root directory
    if (!root.openRoot(vol)) {
        putstring_nl("Can't open root dir!"); // Something went wrong,
        while(1);                             // then 'halt' - do nothing!
    }
    
    // Whew! We got past the tough parts.
    putstring_nl("Ready!");
    
    TCCR2A = 0;
    TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;
  
    //Timer2 Overflow Interrupt Enable
    TIMSK2 |= 1<<TOIE2;
}

void loop() {
    unsigned long currentMillis = millis();
    byte i;
    static int g = 0;
    static int d = 0;
    // Start up power cell
    if (justpressed[0]) {
        justpressed[0] = 0;
	playfile("on.wav");
    }
    // Shut down power cell
    else if (justreleased[0]) {
        justreleased[0] = 0;
        playfile("pwroff.wav");
        // ******************************** powerdown();
        for (int j = 14; j > -1; j--) {
            if (currentMillis - previousMillis_off > off_interval) {
                previousMillis_off = currentMillis;
                // load the light sequence you want from array
    	        dataPWRCL1 = daPWRCL1[j];
    	        dataPWRCL2 = daPWRCL2[j];
    
                // ground latchPin and hold low for transmitting.
                digitalWrite(latchPin, 0);
                // Send data to 595's.
                shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);
                shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
                // return the latch pin high to signal chip that it 
                // no longer needs to listen for information
                digitalWrite(latchPin, 1);
                // ***** delay(30);
            }
        }
        digitalWrite(latchPin, 0);
        shiftOut(dataPinGUN, clockPinGUN, 0x40);
        shiftOut(dataPinGUN, clockPinGUN, 0x00);
        digitalWrite(latchPin, 1);
        
        digitalWrite(latchPin, LOW);
    }
					
    // Start up powercell and thrower (gun) lights when the pack is powered on.
    if (pressed[0] == HIGH) {
        for (int j = 0; j < 15; j++) {
            if (currentMillis - previousMillis_on > pwrcl_interval) {
                // save the last time you blinked the LED
                previousMillis_on = currentMillis;
                //load the light sequence you want from array
      	        dataPWRCL1 = daPWRCL1[j];
      	        dataPWRCL2 = daPWRCL2[j];
    				
                // THIS WILL SET GUN ARRAY TO HOW EVER MANY YOU HAVE IN YOUR ARRAY
    	        if (g > 25) {
                    g = 0;
                }
    	        dataGUN1 = daGUN1[g];
    	        dataGUN2 = daGUN2[g];
    	        g++;
    						
    	        //ground latchPin and hold low for as long as you are transmitting
                digitalWrite(latchPin, 0);
    	        //move 'em out
    	        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);
    	        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
    	        shiftOut(dataPinGUN, clockPinGUN, dataGUN2);
    	        shiftOut(dataPinGUN, clockPinGUN, dataGUN1);
    	        //return the latch pin high to signal chip that it 
    	        //no longer needs to listen for information
                digitalWrite(latchPin, 1);
    	        // ***** delay(60);
            }
        }  	
    }


    // Thrower activation - checks if gun is ON if not it will not do anything.
    if (pressed[0] == HIGH) {
        // if the fire button is pressed 
        if (pressed[1]) {
            playfile("gun2.wav");
          	while (wave.isplaying && pressed[1]) {
                    for (int j = 0; j < 15; j++) {
                        if (currentMillis - previousMillis_fire > firing_interval) {
                            previousMillis_fire = currentMillis;
              	            //load the light sequence you want from array
              		    dataPWRCL1 = daPWRCL1[j];
        		    dataPWRCL2 = daPWRCL2[j];
        		    dataGUNON1 = daGUNON1[j];
        		    dataGUNON2 = daGUNON2[j];
        		    //ground latchPin and hold low for as long as you are transmitting
        		    digitalWrite(latchPin, 0);
        		    //move 'em out
        		    shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);   
        		    shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
        		    shiftOut(dataPinGUN, clockPinGUN, dataGUNON2);   
        		    shiftOut(dataPinGUN, clockPinGUN, dataGUNON1);
        
        		    //return the latch pin high to signal chip that it 
        		    //no longer needs to listen for information
        		    digitalWrite(latchPin, 1);
        		    // ***** delay(40);
                        }
                    }
                }
            wave.stop();
        }
        //play power down file 
        else if (justreleased[1]) {
            justreleased[1] = 0;
    	    playfile("off.wav");
        }
    }
    else {
        // Gun is not on
    }

    //count and increment X. this will play music and you can hit the button to go to the next song. 
    static int x = 0;
    if (justpressed[2]) {
        x++;
        if (x == 1) {
            justpressed[2] = 0;
	    playfile("1.wav");
        }
        if (x == 2) {
            justpressed[2] = 0;
    	    playfile("2.wav");
        }
        if (x == 3) {
            justpressed[2] = 0;
    	    playfile("3.wav");
        }
        if (x == 4) {
    	    justpressed[2] = 0;
    	    playfile("4.wav");
        }
        if (x == 5) {
            justpressed[2] = 0;
    	    playfile("5.wav");
        }
        if (x == 6) {
            justpressed[2] = 0;
    	    playfile("6.wav");
    	    x = 0;
        }
    }
}


// this handy function will return the number of bytes currently free in RAM, great for debugging!
void sdErrorCheck(void) {
  if (!card.errorCode()) return;
      putstring("\n\rSD I/O error: ");
      Serial.print(card.errorCode(), HEX);
      putstring(", ");
      Serial.println(card.errorData(), HEX);
      while(1);
}


// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
    // call our helper to find and play this name
    playfile(name);
    while (wave.isplaying) {
        // do nothing while its playing
    }
    // now its done playing
}

void playfile(char *name) {
    // see if the wave object is currently doing something
    if (wave.isplaying) {// already playing something, so stop it!
        wave.stop(); // stop it
    }
    // look in the root directory and open the file
    if (!f.open(root, name)) {
        putstring("Couldn't open file ");
        Serial.print(name);
        return;
    }
    // OK read the file and turn it into a wave object
    if (!wave.create(f)) {
        putstring_nl("Not a valid WAV");
        return;
    }
    
    // ok time to play! start playback
    wave.play();
}


// Power-down light sequence.
void powerdown() {
}


// the heart of the program
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
    // This shifts 8 bits out MSB first, 
    // on the rising edge of the clock,
    // clock idles low

    // internal function setup
    int i = 0;
    int pinState;
    pinMode(myClockPin, OUTPUT);
    pinMode(myDataPin, OUTPUT);

    //clear everything out just in case to
    //prepare shift register for bit shifting
    digitalWrite(myDataPin, 0);
    digitalWrite(myClockPin, 0);

    //for each bit in the byte myDataOut?
    //NOTICE THAT WE ARE COUNTING DOWN in our for loop
    //This means that %00000001 or "1" will go through such
    //that it will be pin Q0 that lights. 
    for (i = 7; i >= 0; i--)  {
        digitalWrite(myClockPin, 0);
    
        //if the value passed to myDataOut and a bitmask result 
        // true then... so if we are at i=6 and our value is
        // %11010100 it would the code compares it to %01000000 
        // and proceeds to set pinState to 1.
        if ( myDataOut & (1 << i) ) {
            pinState = 1;
        }
        else {	
            pinState = 0;
        }
    
        //Sets the pin to HIGH or LOW depending on pinState
        digitalWrite(myDataPin, pinState);
        //register shifts bits on upstroke of clock pin  
        digitalWrite(myClockPin, 1);
        //zero the data pin after shift to prevent bleed through
        digitalWrite(myDataPin, 0);
    }
    //stop shifting
    digitalWrite(myClockPin, 0);
}

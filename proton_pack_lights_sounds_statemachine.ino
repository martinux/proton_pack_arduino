/* Proton pack lights sounds v2.0 (2021-10-13)

This code drives two sets of two 74HC595N shift registers. One set controls the 'power cell' LEDs 
of the proton pack, the other set runs the 'bargraph' and two firing LEDs on the thrower/wand. 
My pack's cyclotron lights are controlled by a 4017 decade counter and 555 timer however one could 
use the remaining available pins of the arduino to control the cyclotron.
Please note with this code that I'm not going for total screen accuracy, simply something that 
looks pretty close. However, the patterns should be fairly easy to understand and modify if you 
wish to produce something very authentic to that seen on screen (or even in the video game).
Feel free to use the code in your own project, the only thing I ask is that you share any 
modifications, ideally also on gbfans.com forums.

For an excellent overview of using shift registers with arduino please see this video:
https://www.youtube.com/watch?v=Ys2fu4NINrA

 Martinux
*/

// ===============================================================================================
// ADAFRUIT AUDIO FX BOARD
// ===============================================================================================
// UG to Ground 
// TX to #5 
// RX to #6 
// RST to #4
// ACT to #3
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

#define SFX_TX 5
#define SFX_RX 6
#define SFX_RST 4  // Reset
#define ACT 3
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

// audio track names on soundboard
char power_on_snd[] =     "T00     WAV";
char firing_snd[] =       "T01     WAV";
char stop_firing_snd[] =  "T02     WAV";
char hum_long_snd[] =     "T03     WAV";
char hum_snd[] =          "T03_SH~1WAV";
char power_down_snd[] =   "T04     WAV";
char clickTrack[] =       "T05     WAV";
char charge_snd[] =       "T06     WAV";
char overheat_snd[] =     "T07     WAV";
char vent_snd[] =         "T08     WAV";
char texTrack[] =         "T09     WAV";
char choreTrack[] =       "T10     WAV";
char toolsTrack[] =       "T11     WAV";
char listenTrack[] =      "T12     WAV";
char thatTrack[] =        "T13     WAV";
char neutronizedTrack[] = "T14     WAV";
char boxTrack[] =         "T15     WAV";
char slowTheme[] =        "T16     OGG";

int audio_playing;  // to keep track if a sound is playing.

// ===============================================================================================
// STATE MACHINE PHASE TRACKING
// ===============================================================================================
int current_pack_state = 0;
int previous_pack_state = 0;
int first_power_up = 1;

// ===============================================================================================
// TIMEKEEPING
// ===============================================================================================
unsigned long currentMillis = 0;
unsigned long previousMillis_on = 0;
unsigned long previousMillis_fire = 0;
unsigned long off_interval = 30;
unsigned long pwrcl_interval = 70;     // interval at which to cycle lights (milliseconds).
unsigned long max_firing_interval = 60;
unsigned long min_firing_interval = 40;
unsigned long firing_interval = max_firing_interval;    // interval at which to cycle firing lights (milliseconds).
unsigned long overheat_interval = 200;
int max_wand_overheat_cycles = overheat_interval / min_firing_interval;
int wand_overheat_cycles;

// ===============================================================================================
// BUTTONS
// ===============================================================================================
#define DEBOUNCE 5  // button debouncing.
// Pins 14, 15 and 16 are Analog 0, 1 and 2 respectively.
int power_button = 14;
int firing_button = 15;
int aux_button = 16;  // Currently held in reserve for use with Adafruit audio shield.
int power_button_state;
int firing_button_state;
int aux_button_state;

// ===============================================================================================
// SHIFT REGISTERS
// ===============================================================================================
int clockPinPWR = 7;  // Power cell
int dataPinPWR = 8;  // Power cell
int latchPin = 9;
int clockPinWand = 18;  // (18 is analog pin 4 on arduino)
int dataPinWand = 19;   // (19 is analog pin 5 on arduino)

// data array position trackers
int power_cell_array_pos = 0;
int wand_array_pos = 0;
int wand_active_array_pos = 0;
int overheat_array_pos = 0;
// holders for information we're going to pass to shifting function
byte dataPwrCell;
byte dataWand;
byte dataWandON;

// ===============================================================================================
// Populate light pattern arrays into flash memory to save RAM
// ===============================================================================================
// power cell LEDs 0 through 7 then LEDs 8 through 15 (two shift registers)
const PROGMEM uint16_t PwrCell[] = {B10000000,B00000000,
                                    B11000000,B00000000,
                                    B11100000,B00000000,
                                    B11110000,B00000000,
                                    B11111000,B00000000,
                                    B11111100,B00000000,
                                    B11111110,B00000000,
                                    B11111111,B00000000,
                                    B11111111,B10000000,
                                    B11111111,B11000000,
                                    B11111111,B11100000,
                                    B11111111,B11110000,
                                    B11111111,B11111000,
                                    B11111111,B11111100,
                                    B11111111,B11111110
                                    };

//Proton Wand idle pattern - two shift registers
const PROGMEM uint16_t Wand[] = {B01000000,B00000000,
                                 B01100000,B00000000,
                                 B01110000,B00000000,
                                 B01111000,B00000000,
                                 B01111100,B00000000,
                                 B01111110,B00000000,
                                 B01111111,B10000000,
                                 B01111111,B11000000,
                                 B01111111,B11100000,
                                 B01111111,B11110000,
                                 B01111111,B11111000,
                                 B01111111,B11111100,
                                 B01111111,B11111110,
                                 B01111111,B11111100,
                                 B01111111,B11111000,
                                 B01111111,B11110000,
                                 B01111111,B11100000,
                                 B01111111,B11000000,
                                 B01111111,B10000000,
                                 B01111111,B00000000,
                                 B01111110,B00000000,
                                 B01111100,B00000000,
                                 B01111000,B00000000,
                                 B01110000,B00000000,
                                 B01100000,B00000000,
                                 B01000000,B00000000
                                 };

// Proton Wand firing pattern - two shift registers
// 10000000 and 00000001 used for Wand barrel strobe
const PROGMEM uint16_t WandON[] = {B11000000,B00000010,
                                   B00100000,B00000101,
                                   B10010000,B00001000,
                                   B00001000,B00010001,
                                   B10000100,B00100000,
                                   B00000010,B01000001,
                                   B10000001,B10000000,
                                   B10000001,B10000000,
                                   B10000001,B10000000,
                                   B00000010,B01000001,
                                   B10000100,B00100000,
                                   B00001000,B00010001,
                                   B10010000,B00001000,
                                   B00100000,B00000101,
                                   B11000000,B00000010
                                   };

// Overheat patterns
const PROGMEM uint16_t OHWand1[] = {B00000000,
                                    B01000000,
                                    B01100000,
                                    B01110000,
                                    B01111000,
                                    B01111100,
                                    B01111110,
                                    B01111111,
                                    B01111111,
                                    B01111111,
                                    B01111111,
                                    B01111111,
                                    B01111111,
                                    B01111111
                                    };
const PROGMEM uint16_t OHWand2[] = {B00000000,
                                    B00000000,
                                    B00000000,
                                    B00000000,
                                    B00000000,
                                    B00000000,
                                    B00000000,
                                    B00000000,
                                    B10000000,
                                    B11000000,
                                    B11100000,
                                    B11110000,
                                    B11111000,
                                    B11111100
                                    };                                   
const PROGMEM uint16_t OHPwrCell[] = {B00000000,B00000000,
                                      B01111111,B11111110,
                                      B00000000,B00000000,
                                      B01111111,B11111110,
                                      B00000000,B00000000,
                                      B01111111,B11111110,
                                      B00000000,B00000000,
                                      B01111111,B11111110,
                                      B00000000,B00000000,
                                      B01111111,B11111110,
                                      B00000000,B00000000,
                                      B01111111,B11111110,
                                      B00000000,B00000000,
                                      B01111111,B11111110
                                      };


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
    // set up serial port
    //Serial.begin(9600);
    Serial.begin(115200);
    while (!Serial);  // wait for serial port to connect. Needed for native USB
    
    Serial.print(F("Free RAM: "));
    Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!

    // Make input & enable pull-up resistors on switch pins
    pinMode(power_button, INPUT_PULLUP);
    pinMode(firing_button, INPUT_PULLUP);
    pinMode(aux_button, INPUT_PULLUP);

    // Set pins for shift registers:
    pinMode(latchPin, OUTPUT);      // shared between power cell and thrower
    pinMode(clockPinPWR, OUTPUT);
    pinMode(clockPinWand, OUTPUT);
    pinMode(dataPinPWR, OUTPUT);
    pinMode(dataPinWand, OUTPUT);
    // Initialise Power cell and Wand lights
    initialise_lights();

    // softwareserial at 9600 baud
    ss.begin(9600);
    // can also do Serial1.begin(9600)
    
    if (!sfx.reset()) {
    Serial.println("Not found");
        while (1);
    }
    Serial.println("SFX board found");
    uint8_t files = sfx.listFiles();
    
    Serial.println("File Listing");
    Serial.println("========================");
    Serial.println();
    Serial.print("Found "); Serial.print(files); Serial.println(" Files");
    for (uint8_t f = 0; f < files; f++) {
        Serial.print(f); 
        Serial.print("\tname: "); Serial.print(sfx.fileName(f));
        Serial.print("\tsize: "); Serial.println(sfx.fileSize(f));
    }
    Serial.println("========================");
}


void loop() {
    audio_playing = digitalRead(ACT);  // False if audio playing.
    pack_state();  // all the "magic" is done in this function
    if (previous_pack_state != current_pack_state) {
        Serial.print(F("STATE: "));
        Serial.println(current_pack_state);
    }
}


// ===============================================================================================
// State machine
// ===============================================================================================
void pack_state() {
    previous_pack_state = current_pack_state;
    switch (current_pack_state) {
        case 0:  // Pack default state & reset variables
            //overheat_array_pos = 0;
            firing_interval = max_firing_interval;
            wand_overheat_cycles = max_wand_overheat_cycles;
            current_pack_state = 1;  // next cycle move onto Start state
            break;
        
        case 1:  // Start
            power_button_state = digitalRead(power_button);
            if (power_button_state == LOW) {  // power switch pressed.
                if (first_power_up) {
                    playAudio(power_on_snd, audio_playing);
                    Serial.println(F("First power on of pack."));
                    first_power_up = 0;
                }
                else {
                    current_pack_state = 2;
                }
            }
            break;
        
        case 2: // Powered on
            power_button_state = digitalRead(power_button);
            firing_button_state = digitalRead(firing_button);
            // After playing power up play pack hum on loop:
            if ( (power_button_state == LOW) && (audio_playing == 1) ) {
                 playAudio(hum_snd, audio_playing);
            }
            if (power_button_state == HIGH) {
                playAudio(power_down_snd, audio_playing);
                first_power_up = 1;
                current_pack_state = 3;  // Next cycle power down.
            }
            if (firing_button_state == LOW) {
                playAudio(firing_snd, audio_playing);
                current_pack_state = 4;  // Next cycle run firing routine.
            }
            powered_on();
            break;

        case 3: // Power down
            powered_down();
            break;

        case 4: // Firing
            power_button_state = digitalRead(power_button);
            firing_button_state = digitalRead(firing_button);
            if ((power_button_state == HIGH) && (firing_button_state == LOW)) {
                current_pack_state = 3;
            }
            if (firing_button_state == HIGH) {
                playAudio(stop_firing_snd, audio_playing);
                current_pack_state = 5;
            }
            firing();
            break;

        case 5: // End firing
            current_pack_state = 2;
            break;
 
        case 6: // Overheat
            power_button_state = digitalRead(power_button);
            firing_button_state = digitalRead(firing_button);
            if (firing_button_state == HIGH) {
                playAudio(stop_firing_snd, audio_playing);
                overheat_array_pos = 0;
                current_pack_state = 2;
            }
            overheat();
            break;

        case 7: // Overheat lockout (pack shuts down until vented)
            firing_button_state = digitalRead(firing_button);
            if (firing_button_state == HIGH) {
                playAudio(vent_snd, audio_playing);
                if (audio_playing == 1) {
                     current_pack_state = 0;
                }
            }
            break;
            
    }
}


// ===============================================================================================
// Pack states
// ===============================================================================================
void initialise_lights() {
    digitalWrite(latchPin, 0);
    shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, B11000000);  // First 8 LEDs
    shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, B00000000);  // Second 8 LEDs
    shiftOut(dataPinWand, clockPinWand, MSBFIRST, B01000000);
    shiftOut(dataPinWand, clockPinWand, MSBFIRST, B00000000);
    digitalWrite(latchPin, 1);
}


void powered_on() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > pwrcl_interval) {
        previousMillis_on = currentMillis;
        digitalWrite(latchPin, 0);  // Low to send data
        
        // Load the light sequence from arrays then send data to shift registers:
        dataPwrCell = pgm_read_word_near(PwrCell + power_cell_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        power_cell_array_pos++;
        dataPwrCell = pgm_read_word_near(PwrCell + power_cell_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        
        dataWand = pgm_read_word_near(Wand + wand_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWand);
        wand_array_pos++;
        dataWand = pgm_read_word_near(Wand + wand_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWand);
        
        digitalWrite(latchPin, 1);  // high to make shift register output data.
        
        power_cell_array_pos++;
        if (power_cell_array_pos > 29) {
             power_cell_array_pos = 0;
        }
        wand_array_pos++;
        if (wand_array_pos > 49) {
            wand_array_pos = 0;
        }
    }
}


void powered_down() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > off_interval) {
        previousMillis_on = currentMillis;
        if (wand_array_pos != power_cell_array_pos) {
            wand_array_pos = power_cell_array_pos;
        }

        digitalWrite(latchPin, 0);  // Low to send data
        // Load the light sequence from arrays then send data to shift registers:
        dataPwrCell = pgm_read_word_near(PwrCell + power_cell_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        dataPwrCell = pgm_read_word_near(PwrCell + (power_cell_array_pos + 1));
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        
        dataWand = pgm_read_word_near(Wand + wand_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWand);
        dataWand = pgm_read_word_near(Wand + (wand_array_pos + 1));
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWand);
        digitalWrite(latchPin, 1);  // high to make shift register show data.
        
        power_cell_array_pos -= 2;
        if (power_cell_array_pos < 0) {
            power_cell_array_pos = 0;
            current_pack_state = 0;
        }
        wand_array_pos -= 2;
        if (wand_array_pos < 0) {
            wand_array_pos = 0;
            current_pack_state = 0;
        }
    }
}


void firing() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > firing_interval) {
        previousMillis_on = currentMillis;
        digitalWrite(latchPin, 0);  // Low to send data
        
        dataPwrCell = pgm_read_word_near(PwrCell + power_cell_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        power_cell_array_pos++;
        dataPwrCell = pgm_read_word_near(PwrCell + power_cell_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);

        dataWandON = pgm_read_word_near(WandON + wand_active_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWandON);
        wand_active_array_pos++;
        dataWandON = pgm_read_word_near(WandON + wand_active_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWandON);
        
        digitalWrite(latchPin, 1);  // high to make shift register show data.
        
        power_cell_array_pos++;
        if (power_cell_array_pos > 28) {
            firing_interval--;
            Serial.print(F("Firing interval: "));
            Serial.println(firing_interval);
            power_cell_array_pos = 0;
        }
        wand_active_array_pos++;
        if (wand_active_array_pos > 28) {
            wand_active_array_pos = 0;
        }
        if (firing_interval == min_firing_interval) {
            playAudio(overheat_snd, audio_playing);
            wand_active_array_pos = 0;
            current_pack_state = 6;
        }
    }
}


void overheat() {
    currentMillis = millis();
    if (currentMillis - previousMillis_fire > firing_interval) {
        previousMillis_fire = currentMillis;
        digitalWrite(latchPin, 0);  // Low to send data

        dataWandON = pgm_read_word_near(WandON + wand_active_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWandON);
        wand_active_array_pos++;
        dataWandON = pgm_read_word_near(WandON + wand_active_array_pos);
        shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWandON);
        
        digitalWrite(latchPin, 1);  // high to make shift register show data.

        wand_active_array_pos++;
        if (wand_active_array_pos > 28) {
            wand_active_array_pos = 0;
        }
    }
    if (currentMillis - previousMillis_on > overheat_interval) {
        previousMillis_on = currentMillis;
        /*
        // 5x wand cycles for every powercell cycle
        for (wand_overheat_cycles = 0; wand_overheat_cycles < max_wand_overheat_cycles; wand_overheat_cycles++) {
            Serial.print("wand_overheat_cycles: ");
            Serial.println(wand_overheat_cycles);
            digitalWrite(latchPin, 0);  // Low to send data
            
            dataWand = pgm_read_word_near(OHWand1 + wand_active_array_pos);
            Serial.print(dataWand, BIN);
            Serial.print(F("|"));
            if (wand_overheat_cycles % 2 == 0) { // flip the strobe every other cycle
                dataWand ^= 1 << 7;  // Red firing LED
            }
            Serial.print(dataWand, BIN);
            shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWand);
            
            Serial.print(F(" || "));
            
            dataWand = pgm_read_word_near(OHWand2 + wand_active_array_pos);
            Serial.print(dataWand, BIN);
            Serial.print(F("|"));
            if (wand_overheat_cycles % 2 == 1) { // flip the strobe every other cycle
                dataWand ^= 1 << 0;  // Blue firing LED
            }
            Serial.println(dataWand, BIN);
            shiftOut(dataPinWand, clockPinWand, MSBFIRST, dataWand);
            
            digitalWrite(latchPin, 1);  // high to make shift register show data.
        }
        wand_active_array_pos++;
        Serial.print(F("wand_active_array_pos: "));
        Serial.println(wand_active_array_pos);
        if (wand_active_array_pos > 28) {
            wand_active_array_pos = 0;
        }
        */
        

        digitalWrite(latchPin, 0);  // Low to send data
        
        // Load the light sequence from arrays then send data to shift registers:
        dataPwrCell = pgm_read_word_near(OHPwrCell + overheat_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        overheat_array_pos++;
        dataPwrCell = pgm_read_word_near(OHPwrCell + overheat_array_pos);
        shiftOut(dataPinPWR, clockPinPWR, MSBFIRST, dataPwrCell);
        
        digitalWrite(latchPin, 1);  // high to make shift register show data.

        overheat_array_pos++;
        if (overheat_array_pos > 26) {
            Serial.println(F("Finished overheat cycle."));
            overheat_array_pos = 0;
            playAudio(stop_firing_snd, audio_playing);
            initialise_lights();
            current_pack_state = 7;
        }
    }
}


void playAudio( char* trackname, int audio_playing ) {
  // stop track if one is going
  if (audio_playing == 0) {
    sfx.stop();
  }

  // now go play
  if (sfx.playTrack(trackname)) {
    sfx.unpause();
  }
}

// Proton pack lights code.

// ===============================================================================================
// STATE MACHINE
// ===============================================================================================
int current_state = 0;
int previous_state = 0;

// ===============================================================================================
// TIMEKEEPING
// ===============================================================================================
unsigned long currentMillis = 0;
unsigned long previousMillis_on = 0;
unsigned long previousMillis_fire = 0;
unsigned long off_interval = 30;
unsigned long pwrcl_interval = 70;     // interval at which to cycle lights (milliseconds).
unsigned long firing_interval = 60;    // interval at which to cycle firing lights (milliseconds).
unsigned long max_firing_interval = 50;
unsigned long min_firing_interval = 30;
unsigned long overheat_interval = 200;
int overheat_cycles = 7;
int overheat_counter = overheat_cycles;

// ===============================================================================================
// BUTTONS
// ===============================================================================================
#define DEBOUNCE 5  // button debouncing.
// Pins 14, 15 and 16 are Analog 0, 1 and 2 respectively.
int power_button = 14;
int firing_button = 15;
int music_button = 16;  // Currently held in reserve for use with Adafruit audio shield.
int power_button_state;
int firing_button_state;
int music_button_state;

// ===============================================================================================
// SHIFT REGISTERS
// ===============================================================================================
int latchPin = 8;
int clockPinPWR = 6;  // Power cell
int clockPinGUN = 18;  // Gun  (18 is analog pin 4 on arduino)
////Pin connected to DS of 74HC595 (pin 14)
int dataPinPWR = 7;  // Power cell
int dataPinGUN = 19; // Gun (19 is analog pin 5 on arduino)

//holders for information we're going to pass to shifting function
byte dataPWRCL1;
byte dataPWRCL2;
byte dataGUN1;
byte dataGUN2;
byte dataGUNON1;
byte dataGUNON2;
// data arrays
byte PWRCL1[15];
byte PWRCL2[15];
byte GUN1[26];
byte GUN2[26];
byte GUNON1[15];
byte GUNON2[15];
byte OHPWRCL1[2];
byte OHPWRCL2[2];
byte OHGUN1[2];
byte OHGUN2[2];
// track array position
int power_cell_array_pos = 0;
int gun_array_pos = 0;
int gunactive_array_pos = 0;
int overheat_array_pos = 0;


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
    Serial.begin(9600);
    Serial.print("Free RAM: ");
    Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!

    // Make input & enable pull-up resistors on switch pins
    Serial.println("Pull ups on switches");
    digitalWrite(power_button, HIGH);
    digitalWrite(firing_button, HIGH);
    digitalWrite(music_button, HIGH);

    // Populate light pattern arrays
    Serial.println("Populating light arrays");
    // power cell LEDs 0 through 7 then LEDs 8 through 15 (two shift registers)
    PWRCL1[0] = B10000000;  PWRCL2[0] = B00000000;  //10000000 00000000
    PWRCL1[1] = B11000000;  PWRCL2[1] = B00000000;  //11000000 00000000
    PWRCL1[2] = B11100000;  PWRCL2[2] = B00000000;  //11100000 00000000
    PWRCL1[3] = B11110000;  PWRCL2[3] = B00000000;  //11110000 00000000
    PWRCL1[4] = B11111000;  PWRCL2[4] = B00000000;  //11111000 00000000
    PWRCL1[5] = B11111100;  PWRCL2[5] = B00000000;  //11111100 00000000
    PWRCL1[6] = B11111110;  PWRCL2[6] = B00000000;  //11111110 00000000
    PWRCL1[7] = B11111111;  PWRCL2[7] = B00000000;  //11111111 00000000
    PWRCL1[8] = B11111111;  PWRCL2[8] = B10000000;  //11111111 10000000
    PWRCL1[9] = B11111111;  PWRCL2[9] = B11000000;  //11111111 11000000
    PWRCL1[10] = B11111111; PWRCL2[10] = B11100000; //11111111 11100000
    PWRCL1[11] = B11111111; PWRCL2[11] = B11110000; //11111111 11110000
    PWRCL1[12] = B11111111; PWRCL2[12] = B11111000; //11111111 11111000
    PWRCL1[13] = B11111111; PWRCL2[13] = B11111100; //11111111 11111100
    PWRCL1[14] = B11111111; PWRCL2[14] = B11111110; //11111111 11111110
    
    //Proton wand idle pattern - two shift registers
    GUN1[0] = B01000000;  GUN2[0] = B00000000;  //01000000 00000000
    GUN1[1] = B01000000;  GUN2[1] = B00000000;  //01000000 00000000
    GUN1[2] = B01100000;  GUN2[2] = B00000000;  //01100000 00000000
    GUN1[3] = B01110000;  GUN2[3] = B00000000;  //01110000 00000000
    GUN1[4] = B01111000;  GUN2[4] = B00000000;  //01111000 00000000
    GUN1[5] = B01111100;  GUN2[5] = B00000000;  //01111100 00000000
    GUN1[6] = B01111110;  GUN2[6] = B00000000;  //01111110 00000000
    GUN1[7] = B01111111;  GUN2[7] = B10000000;  //01111111 10000000
    GUN1[8] = B01111111;  GUN2[8] = B11000000;  //01111111 11000000
    GUN1[9] = B01111111;  GUN2[9] = B11100000;  //01111111 11100000
    GUN1[10] = B01111111; GUN2[10] = B11110000; //01111111 11110000
    GUN1[11] = B01111111; GUN2[11] = B11111000; //01111111 11111000
    GUN1[12] = B01111111; GUN2[12] = B11111100; //01111111 11111100
    GUN1[13] = B01111111; GUN2[13] = B11111110; //01111111 11111110
    GUN1[14] = B01111111; GUN2[14] = B11111100; //01111111 11111100
    GUN1[15] = B01111111; GUN2[15] = B11111000; //01111111 11111000
    GUN1[16] = B01111111; GUN2[16] = B11110000; //01111111 11110000
    GUN1[17] = B01111111; GUN2[17] = B11100000; //01111111 11100000
    GUN1[18] = B01111111; GUN2[18] = B11000000; //01111111 11000000
    GUN1[19] = B01111111; GUN2[19] = B10000000; //01111111 10000000
    GUN1[20] = B01111111; GUN2[20] = B00000000; //01111111 00000000
    GUN1[21] = B01111110; GUN2[21] = B00000000; //01111110 00000000
    GUN1[22] = B01111100; GUN2[22] = B00000000; //01111100 00000000
    GUN1[23] = B01111000; GUN2[23] = B00000000; //01111000 00000000
    GUN1[24] = B01110000; GUN2[24] = B00000000; //01110000 00000000
    GUN1[25] = B01100000; GUN2[25] = B00000000; //01100000 00000000
    GUN1[26] = B01000000; GUN2[26] = B00000000; //01000000 00000000
    
    // Proton wand firing pattern - two shift registers
    // 10000000 and 00000001 used for GUN strobe
    GUNON1[0] = B11000000;  GUNON2[0] = B00000010;  //1|1000000  0000001|0
    GUNON1[1] = B00100000;  GUNON2[1] = B00000101;  //0|0100000  0000010|1
    GUNON1[2] = B00010000;  GUNON2[2] = B00001000;  //1|0010000  0000100|0
    GUNON1[3] = B10001000;  GUNON2[3] = B00010001;  //0|0001000  0001000|1
    GUNON1[4] = B00000100;  GUNON2[4] = B00100000;  //1|0000100  0010000|0
    GUNON1[5] = B00000010;  GUNON2[5] = B01000001;  //0|0000010  0100000|1
    GUNON1[6] = B10000001;  GUNON2[6] = B10000000;  //1|0000001  1000000|0
    GUNON1[7] = B00000001;  GUNON2[7] = B10000001;  //0|0000001  1000000|1
    GUNON1[8] = B00000001;  GUNON2[8] = B10000000;  //1|0000001  1000000|0
    GUNON1[9] = B10000010;  GUNON2[9] = B01000001;  //0|0000010  0100000|1
    GUNON1[10] = B00000100; GUNON2[10] = B00100000; //1|0000100  0010000|0
    GUNON1[11] = B00001000; GUNON2[11] = B00010001; //0|0001000  0001000|1
    GUNON1[12] = B10010000; GUNON2[12] = B00001000; //1|0010000  0000100|0
    GUNON1[13] = B00100000; GUNON2[13] = B00000101; //0|0100000  0000010|1
    GUNON1[14] = B01000000; GUNON2[14] = B00000010; //1|1000000  0000001|0

    // Overheat patterns
    OHPWRCL1[0] = B00000000;  OHPWRCL2[0] = B00000000;
    OHPWRCL1[1] = B11111111;  OHPWRCL2[1] = B11111111;
    OHGUN1[0] = B00000000;  OHGUN2[0] = B00000000;
    OHGUN1[1] = B01111111;  OHGUN2[1] = B11111110;

    // Set pins for shift registers:
    pinMode(latchPin, OUTPUT);      // shared between power cell and thrower
    pinMode(clockPinPWR, OUTPUT);
    pinMode(clockPinGUN, OUTPUT);
    pinMode(dataPinPWR, OUTPUT);
    pinMode(dataPinGUN, OUTPUT);
    // Initialise Power cell and gun lights
    Serial.println("Initialising lights.");
    digitalWrite(latchPin, 0);
    shiftOut(dataPinPWR, clockPinPWR, 0x00);
    shiftOut(dataPinPWR, clockPinPWR, 0x00);
    shiftOut(dataPinGUN, clockPinGUN, 0x40);
    shiftOut(dataPinGUN, clockPinGUN, 0x00);
    digitalWrite(latchPin, 1);

}


void loop() {
    pack_state();
    if (previous_state != current_state) {
        Serial.print("STATE: ");
        Serial.println(current_state);
    }
}


void pack_state() {
    previous_state = current_state;
    switch (current_state) {
        case 0:  // reset
            current_state = 1;
            pwrcl_interval = 70;
            firing_interval = max_firing_interval;
            overheat_counter = overheat_cycles;
            break;
        
        case 1:  // Start
            power_button_state = digitalRead(power_button);
            if (power_button_state == LOW) {
                current_state = 2;
            }
            else {
                current_state = 0;
            }
            break;
        
        case 2: // Powered on
            power_button_state = digitalRead(power_button);
            firing_button_state = digitalRead(firing_button);
            if (power_button_state == HIGH) {
                current_state = 3;
            }
            if (firing_button_state == LOW) {
                current_state = 4;
            }
            powered_on();
            break;

        case 3: // Power down
            Serial.print("array positions: ");
            Serial.print(power_cell_array_pos);
            Serial.print(" : ");
            Serial.println(gun_array_pos);
            powered_down();
            break;

        case 4: // Firing
            power_button_state = digitalRead(power_button);
            firing_button_state = digitalRead(firing_button);
            //Serial.print("Button states [power|firing]: "); 
            //Serial.print(power_button_state); Serial.print("|"); Serial.println(firing_button_state);
            if ((power_button_state == HIGH) && (firing_button_state == LOW)) {
                current_state = 3;
            }
            if (firing_button_state == HIGH) {
                current_state = 0;
            }
            firing();
            break;

        case 5: // Overheat
            overheat();
            break;

        case 6: // Overheat lockout
            firing_button_state = digitalRead(firing_button);
            if (firing_button_state == HIGH) {
                current_state = 0;
            }
            break;
    }
}


void powered_on() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > pwrcl_interval) {
        previousMillis_on = currentMillis;
        // Load the light sequence from arrays
        dataPWRCL1 = PWRCL1[power_cell_array_pos];
        dataPWRCL2 = PWRCL2[power_cell_array_pos];
        dataGUN1 = GUN1[gun_array_pos];
        dataGUN2 = GUN2[gun_array_pos];

        digitalWrite(latchPin, 0);  // Low to send data
        // Send data to shift registers:
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
        shiftOut(dataPinGUN, clockPinGUN, dataGUN1);
        shiftOut(dataPinGUN, clockPinGUN, dataGUN2);
        digitalWrite(latchPin, 1);  // high to make shift register output data.
        
        power_cell_array_pos++;
        if (power_cell_array_pos > 14) {
             power_cell_array_pos = 0;
        }
        gun_array_pos++;
        if (gun_array_pos > 25) {
            gun_array_pos = 0;
        }
    }
}


void powered_down() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > off_interval) {
        previousMillis_on = currentMillis;
        if (gun_array_pos != power_cell_array_pos) {
            gun_array_pos = power_cell_array_pos;
        }
        // Load the light sequence from arrays
        Serial.print("PWR array: "); Serial.print(power_cell_array_pos);
        Serial.print("  GUN array: "); Serial.println(gun_array_pos);
        
        dataPWRCL1 = PWRCL1[power_cell_array_pos];
        dataPWRCL2 = PWRCL2[power_cell_array_pos];
        dataGUN1 = GUN1[gun_array_pos];
        dataGUN2 = GUN2[gun_array_pos];

        digitalWrite(latchPin, 0);  // Low to send data
        // Send data to shift registers:
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
        shiftOut(dataPinGUN, clockPinGUN, dataGUN1);
        shiftOut(dataPinGUN, clockPinGUN, dataGUN2);
        digitalWrite(latchPin, 1);  // high to make shift register show data.
        
        power_cell_array_pos--;
        if (power_cell_array_pos < 0) {
            power_cell_array_pos = 0;
            current_state = 0;
        }
        gun_array_pos--;
        if (gun_array_pos < 0) {
            gun_array_pos = 0;
            current_state = 0;
        }
    }
}


void firing() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > firing_interval) {
        previousMillis_on = currentMillis;
        // Load the light sequence from arrays
        dataPWRCL1 = PWRCL1[power_cell_array_pos];
        dataPWRCL2 = PWRCL2[power_cell_array_pos];
        dataGUNON1 = GUNON1[gunactive_array_pos];
        dataGUNON2 = GUNON2[gunactive_array_pos];

        digitalWrite(latchPin, 0);  // Low to send data
        // Send data to shift registers:
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
        shiftOut(dataPinGUN, clockPinGUN, dataGUNON1);
        shiftOut(dataPinGUN, clockPinGUN, dataGUNON2);
        digitalWrite(latchPin, 1);  // high to make shift register show data.
        
        power_cell_array_pos++;
        if (power_cell_array_pos > 14) {
            firing_interval--;
            power_cell_array_pos = 0;
        }
        gunactive_array_pos++;
        if (gunactive_array_pos > 14) {
            gunactive_array_pos = 0;
        }
        Serial.print("firing intervals: ");
        Serial.print(firing_interval); Serial.print(" | "); Serial.println(min_firing_interval);
        if (firing_interval < min_firing_interval) {
            current_state = 5;
        }
    }
}


void overheat() {
    currentMillis = millis();
    if (currentMillis - previousMillis_on > overheat_interval) {
        previousMillis_on = currentMillis;
        overheat_array_pos = 1 - overheat_array_pos;
        Serial.print("Overheat array pos: "); Serial.println(overheat_array_pos);
        // Load the light sequence from arrays
        dataPWRCL1 = OHPWRCL1[overheat_array_pos];
        dataPWRCL2 = OHPWRCL2[overheat_array_pos];
        dataGUN1 = OHGUN1[overheat_array_pos];
        dataGUN2 = OHGUN2[overheat_array_pos];

        digitalWrite(latchPin, 0);  // Low to send data
        // Send data to shift registers:
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL1);
        shiftOut(dataPinPWR, clockPinPWR, dataPWRCL2);
        shiftOut(dataPinGUN, clockPinGUN, dataGUN1);
        shiftOut(dataPinGUN, clockPinGUN, dataGUN2);
        digitalWrite(latchPin, 1);  // high to make shift register show data.

        overheat_counter--;
        if (overheat_counter < 0) {
            current_state = 6;
        }
    }
    
}


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

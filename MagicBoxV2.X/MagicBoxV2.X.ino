/* Firmware for MagicBox V2.X - 11/18/2021
  by Jim Kortge K8IQY and Mike Maiorana KU4QO
  Down button:
    quick press - reduce cw speed
    long press  - tune mode turns on transmitter for 10 seconds. Cancel by pressing again.
  Up button:
    quick press - increase cw speed
    long press  - save current cw speed to EEPROM. Use at power up.
    double quick press - announce current cw speed with the sidetone. Sent at current cw speed.
  Non-QSK jumper disables full QSK and adds a timer to revert to RX mode after last TX.
  10k pot with wiper connected to A7 can adjust cw speed from 0 (straight key) to 35wpm
*/
#define VERSION 12062021

#include <EEPROM.h>

//set up for control lines
#define SPD_UP        4         //CW speed Up button, momentary SPST, 0=closed
#define SPD_DWN       5         //CW speed Down button, momentary SPST, 0=closed
#define NON_QSK       6         //Non QSK header, open=QSK, closed=Non-QSK
#define SIDETONE      7         //Side tone signal control, 1=on
#define RX_MUTE       8         //Shunt switch in Rx audio path, 1=on
#define PADDLE_DIT    9         //Paddle dit contact, 0=closed
#define PADDLE_DAH    10        //Paddle dah contact, 0=closed 
#define RX_PIN        11        //Rx PIN diode path enable, 1=on, This line also controls the Rx audio path series switches
#define TX_PIN        12        //Tx PIN diode path enable, 1=on
#define TX_ENABLE     13        //Transmitter ON/OFF control line, 1=on
#define SPD_DIAL      A7        //Input from cw speed potentiometer

//keyercontrol flags
#define     DIT_L      0x01     // Dit latch
#define     DAH_L      0x02     // Dah latch
#define     DIT_PROC   0x04     // Dit is being processed
#define     PDLSWAP    0x08     // 0 for normal, 1 for swap
#define     IAMBICB    0x10     // 0 for Iambic A, 1 for Iambic B

#define EE_KEYER_SPEED  0    // spot to save keyer speed

#define INCR 5                  //cw speed increment/decrement value

//set up for keyer
unsigned long       ditTime;                       // No. milliseconds per dit
unsigned char       keyerControl;
unsigned char       keyerState;
enum KSTYPE {IDLE, CHK_DIT, CHK_DAH, KEYED_PREP, KEYED, INTER_ELEMENT };
int wpm;                                   //holds cw speed
int a_wpm;                                 //analog reading of cw speed dial
int a_wpm_save;                            //holds last dial cw reading
bool straight_key = false;                 // straight key enable
bool sk_on = false;                        //tracking straight key transmit status
static long ktimer;                        // timer variable for keyer

//input switches Finite State Machines
unsigned long t_su = 0;                   //timer for up switch
unsigned long t_sd = 0;                   //timer for down switch
const unsigned long bounce_delay = 10;    //debounce timer in milliseconds
const unsigned long double_delay = 200;   //double-press timeout in milliseconds
const unsigned long hold_delay = 500;     //long-press timeout in milliseconds
const unsigned long qsk_timeout = 1500;   //RX timeout for non-QSK mode in milliseconds
const unsigned long tune_timeout = 10000; //tune mode timer in milliseconds
unsigned long qsk_timer;                  //variable holds non-QSK mode timer
bool qsk_enable;                          //flag to track QSK mode. True is full QSK on.
//enumeration for button state machines 
enum FSM {RST, WAIT, ARM, DEBOUNCE, LIFT, SHORT, ARMDBL, CHKDBL, DBL, LONG, RELEASE, CANCEL, FIN, FIN_WAIT};
FSM state_su;                             //new variable of enumeration type FSM for switch "up"
FSM state_sd;                             //new variable of enumeration type FSM for switch "down"

const int morse[10] {                     //array contains data to send numbers 0-9 in morse. 1=dit, 0=dah
0b00000,
0b00001,
0b00011,
0b00111,
0b01111,
0b11111,
0b11110,
0b11100,
0b11000,
0b10000
};

int int_wpm;                              //integers used in announce routine to send CW speed in morse to sidetone
int int_wpm_l;

void setup () {
  //initialize the I/O pins
  pinMode(SPD_UP, INPUT_PULLUP);
  pinMode(SPD_DWN, INPUT_PULLUP);
  pinMode(NON_QSK, INPUT_PULLUP);
  pinMode(PADDLE_DIT, INPUT_PULLUP);
  pinMode(PADDLE_DAH, INPUT_PULLUP);

  pinMode(TX_ENABLE, OUTPUT);     //this output controls the transmitter
  digitalWrite(TX_ENABLE, LOW);   //make sure it is off
  pinMode(TX_PIN, OUTPUT);        //Tx PIN switch control
  digitalWrite(TX_PIN, LOW);      //Initialize low, disconnect transmitter from antenna
  pinMode(RX_PIN, OUTPUT);        //Rx PIN switch control
  digitalWrite(RX_PIN, HIGH);     //Initialize high, connects antenna to receiver
  pinMode(RX_MUTE,  OUTPUT);      //Rx audio mute line, shunt switch
  digitalWrite(RX_MUTE, LOW);     //Initialize low, so shunt switch is open
  pinMode(SIDETONE, OUTPUT);      //Side tone control line
  digitalWrite(SIDETONE, LOW);    //Initial low, so tone off during receive

  wpm = eeprom_read_dword((const uint32_t *)EE_KEYER_SPEED);    //read saved keyer speed
    if (wpm < 0 | wpm > 35) {
      wpm = 15;                                                //set wpm to 15 if EEPROM value is unreasonable
      eeprom_write_dword((uint32_t *)EE_KEYER_SPEED, wpm);     //save reasonable value to EEPROM
    }
    if (wpm < 5) straight_key = true;                         //set straight key mode if stored wpm < 5
    
    loadWPM(wpm);                                             //set keyer speed  
    a_wpm_save = analogRead(SPD_DIAL) >> 5;                   //get initial dial position
}

void loop()                                         //Main program loop
{

  a_wpm = analogRead(SPD_DIAL) >> 5;                //read potentiometer voltage and set keyer speed
  if (a_wpm != a_wpm_save ) {                       //process dial reading only when it changes
    a_wpm_save = a_wpm;                             //save last analog reading
    if ( a_wpm == 0 ) {
      straight_key=true;
      wpm=5;
    }
    else {
      wpm = a_wpm + 4;
      straight_key=false;
      loadWPM(wpm);
    }
  }
  
  qsk_enable=digitalRead(NON_QSK);                //read non-QSK jumper and set system mode
  
  //Routine to key system, either with keyer or straight key
  if (straight_key == false) check_keyer();      //call keyer or straight key code depending on mode
    else check_sk();

  //CW speed changes using SPD_UP and SPD_DWN switches
  fsm_su();                                      // run switch up state machine
  if ((state_su == SHORT) && (wpm < 35)) {      //if active, increment the wpm speed unless at upper limit
    wpm += INCR;
    quick_beep();
    straight_key = false;                        //make sure straight key mode is off
    loadWPM(wpm);                                //Update the current keyer speed
  }
  if (state_su == FIN_WAIT) {                    // long press of up button saves wpm to EEPROM
    save_keyer();
    quick_beep();
  }
  if ( state_su == DBL) {                        // double press to activate CW speed announcement
    announce();
  }

  fsm_sd();
  if ((state_sd == SHORT) && (wpm > 5)) {        //if active, decrement the wpm speed unless at lower limit
    wpm -= INCR;
    if (wpm < 5) wpm = 5;                        //5wpm minimum
    quick_beep();
    straight_key = false;
    loadWPM(wpm);                                 //Update the current keyer speed
  }
  else if ( (state_sd == SHORT) && (wpm = 5)) {   //if down switch pressed at 5wpm, go to straight key mode
    //wpm -= INCR;
    quick_beep();
    straight_key = true;
    loadWPM(wpm);      
  }
  else if ((state_sd == SHORT) && (wpm < 5)) {  //down switch pressed when already in straight key mode. Do nothing.
    straight_key = true;
  }
  if (state_sd == LONG) xmit_on();              // tune function if down button long press
  if (state_sd == FIN) xmit_off();              // cancel tune function

  if (qsk_enable == false) {                    // check qsk timer in non-QSK mode. Recover RX when timer expires.
    if (((millis() - qsk_timer) > qsk_timeout ) && (digitalRead(RX_MUTE) == HIGH)) rx_on();
  }
}

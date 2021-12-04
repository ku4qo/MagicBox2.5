/* Firmware for MagicBox V2.X - 11/18/2021
  by Jim Kortge K8IQY and Mike Maiorana KU4QO
  Down button:
    quick press - reduce cw speed
    long press  - tune mode turns on transmitter for 10 seconds. Cancel by pressing again.
  Up button:
    quick press - increase cw speed
    long press  - save current cw speed to EEPROM. Use at power up.
    double quick press - announce current cw speed with the sidetone. Sent at current cw speed.
*/
#define VERSION 12032021-2

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

//keyercontrol flags
#define     DIT_L      0x01     // Dit latch
#define     DAH_L      0x02     // Dah latch
#define     DIT_PROC   0x04     // Dit is being processed
#define     PDLSWAP    0x08     // 0 for normal, 1 for swap
#define     IAMBICB    0x10     // 0 for Iambic A, 1 for Iambic B

#define EE_KEYER_SPEED  0    // spot to save keyer speed

//set up for keyer
unsigned long       ditTime;                       // No. milliseconds per dit
unsigned char       keyerControl;
unsigned char       keyerState;
enum KSTYPE {IDLE, CHK_DIT, CHK_DAH, KEYED_PREP, KEYED, INTER_ELEMENT };
int wpm = 15;                                   //Start CW at 15 wpm
bool straight_key = false;
bool sk_on = false;                               //tracking straight key transmit status
unsigned long mt_timer;
static long ktimer;                               // timer variable for keyer

//input switches Finite State Machines
unsigned long t_su = 0;
unsigned long t_sd = 0;
unsigned long bounce_delay = 10;
unsigned long double_delay = 200;
unsigned long hold_delay = 500;
unsigned long tune_timeout = 10000;
enum FSM {RST, WAIT, ARM, DEBOUNCE, LIFT, SHORT, ARMDBL, CHKDBL, DBL, LONG, RELEASE, CANCEL, FIN, FIN_WAIT};
FSM state_su;  //new variable of enumeration type FSM for switch "up"
FSM state_sd;  //new variable of enumeration type FSM for switch "down"

int morse[10] {
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

int int_wpm;
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

  loadWPM(wpm);                // set keyer speed

  wpm = eeprom_read_dword((const uint32_t *)EE_KEYER_SPEED); //read saved keyer speed
    if (wpm < 0 | wpm > 35) {
      wpm = 15;                                                //set wpm to 15 if unreasonable
      eeprom_write_dword((uint32_t *)EE_KEYER_SPEED, wpm);
    }
    if (wpm < 5) { straight_key = true;}
}

void loop()                       //Main program loop
{
  //Routine to key system, either with keyer or straight key
  if (straight_key == false) {
    check_keyer();    //check for keyer operation

  }
  else {
    check_sk();
  }

  //CW speed changes using SPD_UP and SPD_DWN switches
  fsm_su();
  if ((state_su == SHORT) && (wpm <= 35)) {      //if active, increment the wpm speed unless at upper limit
    wpm += 2;
    quick_beep();
    straight_key = false;                              //make sure straight key mode is off
    loadWPM(wpm);                                 //Update the current keyer speed
    //announce();
  }

  if (state_su == FIN_WAIT) {           // long press of up button saves wpm to EEPROM
    save_keyer();
    quick_beep();
  }

  if ( state_su == DBL) {
    announce();
  }

  fsm_sd();
  if ((state_sd == SHORT) && (wpm > 5)) {         //if active, decrement the wpm speed unless at lower limit
    wpm -= 2;
    quick_beep();
    straight_key = false;
    loadWPM(wpm);                                 //Update the current keyer speed
    //announce();
  }
  else if ( (state_sd == SHORT) && (wpm = 5)) {
    wpm -= 2;
    quick_beep();
    straight_key = true;
    loadWPM(wpm); 
    //announce();       
  }
  else if ((state_sd == SHORT) && (wpm < 5)) {
    straight_key = true;
    //announce();
  }
  if (state_sd == LONG) {             // tune function if down button long press
      xmit_on();
    }
    if (state_sd == FIN) {            // cancel tune
      xmit_off();
    }
}

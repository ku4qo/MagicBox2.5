//Transmit ON/OFF control

void xmit_on()                                //Routine to turn the transmitter ON
{
  digitalWrite(RX_MUTE, HIGH);          //Close Shunt switch to mute Rx
  delay(2);
  digitalWrite(RX_PIN, LOW);            //Disconnect Rx from antenna, open audio Rx series switches
  delay(5);
  digitalWrite(TX_PIN, HIGH);           //Connect Tx to antenna
  delay(2);
  digitalWrite(SIDETONE, HIGH);         //Turn on side tone
  delay(2);
  digitalWrite(TX_ENABLE, HIGH);        //Turn on Tx
  if (qsk_enable == false) qsk_timer = millis();    //kick the QSK timer in non-QSK mode
}

void xmit_off()                         //Routine to turn the transmitter OFF
{
  digitalWrite(SIDETONE, LOW);          //Turn off side tone
  delay(2);
  digitalWrite(TX_ENABLE, LOW);         //Turn off Tx
  delay(5);
  if (qsk_enable == true) rx_on();      //recover Rx in QSK mode. Kick timer in non-QSK mode.
  else qsk_timer = millis();
}

void rx_on()                            //Routine to turn the receiver ON
{
  digitalWrite(TX_PIN, LOW);            //Disconnect Tx from antenna
  delay(2);
  digitalWrite(RX_PIN, HIGH);           //turn on Rx PIN diode switch
  delay(2);
  digitalWrite(RX_MUTE, LOW);           //Open shunt switch to unmute Rx audio
}

void quick_beep()                       //short sidetone beep for button feedback
{
    digitalWrite(SIDETONE, HIGH);
    delay(50);
    digitalWrite(SIDETONE, LOW);
}

void announce()                         //announce wpm with sidetone at current wpm setting
{
  if (straight_key == true) {           //in straight key mode, send morse S character
    //send S
    quick_beep();
    delay(50);
    quick_beep();
    delay(50);
    quick_beep();
  }
  else if (wpm >= 10) {                 //two character cw speed. Break up into to separate number for sending
    int_wpm = wpm / 10;
    int_wpm_l= wpm % 10;
    for (int bits = 0; bits < 5; bits++) {            //read one bit at a time and send dit or dah
      if (morse[int_wpm] & (1 << bits)) dit_tone();
        else dah_tone();
    }
    
    delay(ditTime);                     //intercharacter delay
    
    for (int bits = 0; bits < 5; bits++) {      //send lower number in CW
      if (morse[int_wpm_l] & (1 << bits)) dit_tone();
        else dah_tone();
    }
  }
    else {
      int_wpm = wpm;
      for (int bits = 0; bits < 5; bits++) {
      if (morse[int_wpm] & (1 << bits)) dit_tone();
        else dah_tone();
      }
    }
}

void dit_tone()                         //dit sidetone for cw speed announcement
{
    digitalWrite(SIDETONE, HIGH);
    delay(ditTime);
    //delay(120);                       //uncomment these and comment other delays for fixed cw speed announce
    digitalWrite(SIDETONE, LOW);
    delay(ditTime);
    //delay(120);
}

void dah_tone()                       //dah sidetone for cw speed announcement
{
    digitalWrite(SIDETONE, HIGH);
    delay(3 * ditTime);
    //delay(360);
    digitalWrite(SIDETONE, LOW);
    delay(ditTime);
    //delay(120);
}

//CW keyer code for the MagicBox V2.X system

void update_PaddleLatch()
{
  if (digitalRead(PADDLE_DIT) == LOW) {
    keyerControl |= DIT_L;
  }
  if (digitalRead(PADDLE_DAH) == LOW) {
    keyerControl |= DAH_L;
  }
}

void loadWPM (float wpm)
{
  ditTime = 1200 / wpm;
}

void check_keyer()
{
  //keyer control section
  switch (keyerState) {
    case IDLE:
      // Wait for direct or latched paddle press
      if ((digitalRead(PADDLE_DIT) == LOW) ||
          (digitalRead(PADDLE_DAH) == LOW) ||
          (keyerControl & 0x03)) {
        update_PaddleLatch();
        keyerState = CHK_DIT;
      }
      break;

    case CHK_DIT:
      // See if the dit paddle was pressed
      if (keyerControl & DIT_L) {
        keyerControl |= DIT_PROC;
        ktimer = ditTime;
        keyerState = KEYED_PREP;
      }
      else {
        keyerState = CHK_DAH;
      }
      break;

    case CHK_DAH:
      // See if dah paddle was pressed
      if (keyerControl & DAH_L) {
        ktimer = ditTime * 3;
        keyerState = KEYED_PREP;
      }
      else {
        keyerState = IDLE;
      }
      break;

    case KEYED_PREP:
      // Assert key down, start timing, state shared for dit or dah
      xmit_on();
      ktimer += millis();                 // set ktimer to interval end time
      keyerControl &= ~(DIT_L + DAH_L);   // clear both paddle latch bits
      keyerState = KEYED;                 // next state
      break;

    case KEYED:
      // Wait for timer to expire
      if (millis() > ktimer) {          // are we at end of key down ?
        xmit_off();
        ktimer = millis() + ditTime;    // inter-element time
        keyerState = INTER_ELEMENT;     // next state
      }
      else if (keyerControl & IAMBICB) {
        update_PaddleLatch();           // early paddle latch in Iambic B mode
      }
      break;

    case INTER_ELEMENT:
      // Insert time between dits/dahs
      update_PaddleLatch();               // latch paddle state
      if (millis() > ktimer) {            // are we at end of inter-space ?
        if (keyerControl & DIT_PROC) {             // was it a dit or dah ?
          keyerControl &= ~(DIT_L + DIT_PROC);   // clear two bits
          keyerState = CHK_DAH;                  // dit done, check for dah
        }
        else {
          keyerControl &= ~(DAH_L);              // clear dah latch
          keyerState = IDLE;                     // go idle
        }
      }
      break;
  }
  
}

void check_sk(void)
{
  if (digitalRead(PADDLE_DIT) == LOW || digitalRead(PADDLE_DAH) == LOW) {
    if (sk_on == false) {
      xmit_on();
      sk_on = true;
    }
  }
  else {
    if (sk_on == true) {
      xmit_off();
      sk_on = false;
    }
  }
}

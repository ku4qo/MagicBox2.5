//State machine routines for cw speed up and down switches

void fsm_sd() {                               //Control Switch state machine
  switch (state_sd) {
    case RST:
      state_sd = WAIT;
      break;

    case WAIT:
      if (digitalRead(BTN) == LOW) {
        state_sd = ARM;
      }
      break;

    case ARM:
      t_sd = millis();
      state_sd = DEBOUNCE;
      break;

    case DEBOUNCE:
      if (millis() - t_sd > bounce_delay) {
        state_sd = LIFT;                        //if debounce timer expires - good press
      }
      if (digitalRead(BTN) == HIGH) {
        state_sd = RST;                         //bounce, go back to reset state
      }
      break;

    case LIFT:
      if (digitalRead(BTN) == HIGH) {
        state_sd = SHORT;
      }
      if (millis() - t_sd > hold_delay) {state_sd = LONG;}
      break;

    case SHORT:
      state_sd = RST;
      break;

    case LONG:
      state_sd = RELEASE;
      break;

    case RELEASE:
      if (digitalRead(BTN) == HIGH) {
        state_sd = CANCEL;
      }
      break;

    case CANCEL:
      if (qsk_enable == false) qsk_timer = millis();        //kick qsk timer so tune mode doesn't time out Rx
      if (digitalRead(BTN) == LOW) {state_sd = FIN;}
      if (millis() - t_sd > tune_timeout) {state_sd = FIN;}
    break;

    case FIN:
      state_sd = FIN_WAIT;
    break;

    case FIN_WAIT:
      if (digitalRead(BTN) == HIGH) {state_sd = RST;}
      break;
  }
}

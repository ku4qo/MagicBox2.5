//State machine routines for cw speed up and down switches

void fsm_su() {                                 //Switch UP state machine
  switch (state_su) {
    case RST:
      state_su = WAIT;
      break;

    case WAIT:
      if (digitalRead(SPD_UP) == LOW) {
        state_su = ARM;
      }
      break;

    case ARM:
      t_su = millis();
      state_su = DEBOUNCE;
      break;

    case DEBOUNCE:
      if (millis() - t_su > bounce_delay) {
        state_su = LIFT; //if debounce timer expires - good press
      }
      if (digitalRead(SPD_UP) == HIGH) {
        state_su = RST; //bounce, go back to reset state
      }
      break;

    case LIFT:
      if (digitalRead(SPD_UP) == HIGH) {
        state_su = ARMDBL;
      }
      if (millis() - t_su > hold_delay) {state_su = LONG;}
      break;

    case ARMDBL:
      t_su = millis();
      state_su = CHKDBL;
      break;

    case CHKDBL:
      if (millis() - t_su > double_delay) {
        state_su = SHORT;
      }
      if ((millis() - t_su < double_delay) && (digitalRead(SPD_UP) == LOW)) {
        state_su = DBL;
      }
      break;

    case DBL:
      state_su = RST;
      break;
      
    case SHORT:
      state_su = RST;
      break;

    case LONG:
      state_su = RELEASE;
      break;

    case RELEASE:
      if (digitalRead(SPD_UP) == HIGH) {
        state_su = FIN_WAIT;
      }
      break;

    case FIN_WAIT:
      state_su = RST;
      break;
  }
}

void fsm_sd() {                               //Switch DOWN state machine
  switch (state_sd) {
    case RST:
      state_sd = WAIT;
      break;

    case WAIT:
      if (digitalRead(SPD_DWN) == LOW) {
        state_sd = ARM;
      }
      break;

    case ARM:
      t_sd = millis();
      state_sd = DEBOUNCE;
      break;

    case DEBOUNCE:
      if (millis() - t_sd > bounce_delay) {
        state_sd = LIFT; //if debounce timer expires - good press
      }
      if (digitalRead(SPD_DWN) == HIGH) {
        state_sd = RST; //bounce, go back to reset state
      }
      break;

    case LIFT:
      if (digitalRead(SPD_DWN) == HIGH) {
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
      if (digitalRead(SPD_DWN) == HIGH) {
        state_sd = CANCEL;
      }
      break;

    case CANCEL:
      if (digitalRead(SPD_DWN) == LOW) {state_sd = FIN;}
      if (millis() - t_sd > tune_timeout) {state_sd = FIN;}
    break;

    case FIN:
      state_sd = FIN_WAIT;
    break;

    case FIN_WAIT:
      if (digitalRead(SPD_DWN) == HIGH) {state_sd = RST;}
      break;
  }
}

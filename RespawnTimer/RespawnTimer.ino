#include <EEPROM.h>
#include <Bounce2.h>
#include <LiquidCrystal.h>


#define HHC 0 // hours counter index in Counters array
#define MMC 1 // minutes counter index in Counters array
#define SSC 2 // seconds counter index in Counters array
#define FRGC 3 // frags counter index in Counters array

#define BTN_CAL 9 // calibration counter select button port
#define BTN_INC 8 // calibration counter increment button port
#define BTN_DEC 7 // calibration counter decrement button port
#define BTN_START 6 // timer start/frag decrement button port
#define BUZZER 13 // buzzer port

LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);

// logic and timers
boolean CalibrationStarted = false;
boolean TimerStarted = false;
boolean FragsOnly = false;
boolean NoFrags = false;
uint8_t Counters[] = {
  0,0,0,0
};
uint8_t MaxCounters[] = {
  99,59,59,99
};
uint8_t CurrentCounter = 1;
unsigned long FinishTime;
unsigned long NextTick;
unsigned long frgBeep;

// button debouncers
Bounce btn_calibration = Bounce();
Bounce btn_increment = Bounce();
Bounce btn_decrement = Bounce();
Bounce btn_start = Bounce();

void setup() {
  pinMode(BTN_CAL, INPUT);
  digitalWrite(BTN_CAL,HIGH);
  btn_calibration.attach(BTN_CAL);
  btn_calibration.interval(5);

  pinMode(BTN_INC, INPUT);
  digitalWrite(BTN_INC,HIGH);
  btn_increment.attach(BTN_INC);
  btn_increment.interval(5);

  pinMode(BTN_DEC, INPUT);
  digitalWrite(BTN_DEC,HIGH);
  btn_decrement.attach(BTN_DEC);
  btn_decrement.interval(5);

  pinMode(BTN_START, INPUT);
  digitalWrite(BTN_START,HIGH);
  btn_start.attach(BTN_START);
  btn_start.interval(5);

  loadCounters();

  lcd.begin(8, 2);
  lcd.blink();

  pinMode(BUZZER, OUTPUT);
  beep(200);
}

void loop() {
  if(!TimerStarted){
    calibrationMode();
  } 
  else {
    timerMode();
  }  
}

void loadCounters() {
  Counters[HHC] = EEPROM.read(HHC);
  if(Counters[HHC] > MaxCounters[HHC]) { 
    Counters[HHC] = 0; 
  }
  Counters[MMC] = EEPROM.read(MMC);
  if(Counters[MMC] > MaxCounters[MMC]) { 
    Counters[MMC] = 0; 
  }
  Counters[SSC] = EEPROM.read(SSC);
  if(Counters[SSC] > MaxCounters[SSC]) { 
    Counters[SSC] = 0; 
  }
  Counters[FRGC] = EEPROM.read(FRGC);
  if(Counters[FRGC] > MaxCounters[FRGC]) { 
    Counters[FRGC] = 0; 
  }
}

void saveCounters() {
  if(Counters[HHC] != EEPROM.read(HHC)) {
    EEPROM.write(HHC, Counters[HHC]);
  }
  if(Counters[MMC] != EEPROM.read(MMC)) {
    EEPROM.write(MMC, Counters[MMC]);
  }
  if(Counters[SSC] != EEPROM.read(SSC)) {
    EEPROM.write(SSC, Counters[SSC]);
  }
  if(Counters[FRGC] != EEPROM.read(FRGC)) {
    EEPROM.write(FRGC, Counters[FRGC]);
  }
}

void beep(int ms) {
  digitalWrite(BUZZER, HIGH);
  delay(ms);
  digitalWrite(BUZZER, LOW);
}

void timerMode() {
  if(!NoFrags && millis() >= frgBeep) {
    // here we stop the frag decrementation counter beep
    digitalWrite(BUZZER, LOW);
  }
  if(!FragsOnly) {
    if(millis() >= FinishTime) { 
      // clear the final second on screen!
      printNumber(0, 6, 0);
      // beep that time has come!    
      beep(2000);
      resetTimer();
      return;
    } 
    else if(millis() >= NextTick) {
      if(Counters[SSC] == 0) {
        Counters[SSC] = MaxCounters[SSC];
        if(Counters[MMC] == 0) {
          Counters[MMC] = MaxCounters[MMC];
          if(Counters[HHC] > 0) {
            Counters[HHC]--;
            printNumber(Counters[HHC], 0, 0);
          }
        } 
        else {
          Counters[MMC]--;
        }
        printNumber(Counters[MMC], 3, 0);
      } 
      else {
        Counters[SSC]--;
      }
      printNumber(Counters[SSC], 6, 0);
      NextTick = millis() + 1000;
    }
  }
  if(!NoFrags) {
    // handle the frags counting by pushing the start button
    if(btn_start.update() == 1 && btn_start.read() == 0 && Counters[FRGC] > 0) {
      Counters[FRGC]--;
      printNumber(Counters[FRGC], 6, 1);
      // reset if all the frags counted by this time period
      if(Counters[FRGC] == 0) {
        beep(2000);
        resetTimer();
        return;
      } 
      else {
        // just short beep (but without delay) to indicate a new frag counted
        frgBeep = millis() + 200;
        digitalWrite(BUZZER, HIGH);
      }
    }
  }
}

void resetTimer() {
  loadCounters();
  printNumber(Counters[HHC], 0, 0);
  printNumber(Counters[MMC], 3, 0);
  printNumber(Counters[SSC], 6, 0);
  printNumber(Counters[FRGC], 6, 1);
  initTimer();
}

void initTimer() {
  if(!FragsOnly) {
    FinishTime = millis() + (1000 * ((unsigned long)Counters[SSC] + ((unsigned long)Counters[MMC] * 60) + ((unsigned long)Counters[HHC] * 3600)));
    NextTick = millis() + 1000; // get ready for next tick (seconds decrement)
  }
  // here we go to the timer mode!
  TimerStarted = true;
}

void calibrationMode() {
  if(!CalibrationStarted) {
    initCalibration();
  }

  // select next counter
  if(btn_calibration.update() == 1 && btn_calibration.read() == 0) {
    setCurrentCounter((CurrentCounter == FRGC) ? 0 : CurrentCounter + 1);
    return;
  }

  if(btn_increment.update() == 1 && btn_increment.read() == 0) {
    Counters[CurrentCounter] = (Counters[CurrentCounter] == MaxCounters[CurrentCounter]) ? 0 : Counters[CurrentCounter] + 1;    
    printCounter(CurrentCounter);
    return;
  }

  if(btn_decrement.update() == 1 && btn_decrement.read() == 0) {
    Counters[CurrentCounter] = (Counters[CurrentCounter] == 0) ? MaxCounters[CurrentCounter] : Counters[CurrentCounter] - 1;
    printCounter(CurrentCounter);
    return;
  }

  if(btn_start.update() == 1 && btn_start.read() == 0) {
    if(Counters[SSC] == 0 && Counters[MMC] == 0 && Counters[HHC] == 0) {
      if(Counters[FRGC] > 0) {
        FragsOnly = true; // go to frags only mode
      } 
      else { // or just nothing configured
        beep(100);
        delay(100);
        beep(100);
        delay(100);
        beep(100);
        delay(100);
        return;
      }
    }
    NoFrags = Counters[FRGC] == 0;
    // finishing the calibration - removing the cursor
    lcd.noBlink();
    // and beep about it :)
    beep(200);
    saveCounters();
    initTimer();
    return; // next mode is the timer mode
  }
}

void initCalibration() {
  // print initial time calibration string
  printNumber(Counters[HHC], 0, 0);
  lcd.print(":");
  printNumber(Counters[MMC], 3, 0);
  lcd.print(":");
  printNumber(Counters[SSC], 6, 0);

  // print initial frags calibration string
  lcd.setCursor(0, 1);
  lcd.print("FRAGS:");
  printNumber(Counters[FRGC], 6, 1);

  // set the calibration marker (cursor) to minutes by default...
  setCurrentCounter(MMC);

  CalibrationStarted = true;
}

void setCurrentCounter(uint8_t counter) {
  switch(counter) {
  case HHC:
    lcd.setCursor(1, 0);
    break;
  case MMC:
    lcd.setCursor(4, 0);
    break;
  case SSC:
    lcd.setCursor(7, 0);
    break;
  case FRGC:
    lcd.setCursor(7, 1);
    break;
  default:
    return;
  }
  CurrentCounter = counter;
}

void printCounter(uint8_t counter) {
  switch(counter) {
  case HHC:
    printNumber(Counters[HHC], 0, 0);
    break;
  case MMC:
    printNumber(Counters[MMC], 3, 0);
    break;
  case SSC:
    printNumber(Counters[SSC], 6, 0);
    break;
  case FRGC:
    printNumber(Counters[FRGC], 6, 1);
    break;
  default:
    return;
  }
  setCurrentCounter(counter);
}

void printNumber(uint8_t number, uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
  if(number < 10) {
    lcd.print(0);
  }
  lcd.print(number);
}


































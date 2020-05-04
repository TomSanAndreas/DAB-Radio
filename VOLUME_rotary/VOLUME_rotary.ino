

#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(22, 23, 5, 18, 19, 21);

uint8_t Volume = 25;
uint8_t LprevState = 0;
uint8_t LcurState = 0;
uint8_t RprevState = 0;
uint8_t RcurState = 0;
uint8_t channelcounter = 7;
char kannaal[8][15] = { "STUBRU", "QMusic", "Joe", "KLARA", "Radio Maria", "MNM", "Nostalgie", "HitFM" };
String song = "Don't stop me now - Queen";
bool afterVolInterrupt = false;


void setup() {
  lcd.begin(16, 2);
  lcd.print("Welkom");
  pinMode(32, INPUT);
  pinMode(25, INPUT);
  delay(2000);
  lcd.clear();
  lcd.print(kannaal[channelcounter]);
  lcd.setCursor(0, 1);
  lcd.print(song);
  attachInterrupt(digitalPinToInterrupt(32), volume, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(25), volume, CHANGE);
}

void loop() {
  if (afterVolInterrupt) {
    lcd.clear();
    //lcd.print("Volume ");
    //volcheck();
    lcd.print(Volume);
    /*
    delay(1000);
    lcd.clear();
    lcd.print(kannaal[channelcounter]);
    */
    //lcd.setCursor(0, 1);
    //lcd.print(song);
    afterVolInterrupt = false;
  }
}

void volume() {
    volloop();
  afterVolInterrupt = true;
}



void volloop() {
  //delay(5);
  LcurState = digitalRead(25);
  RcurState = digitalRead(32);
  if (LcurState == HIGH && RcurState == HIGH || LcurState == LOW && RcurState == LOW) {
    Volume += 1;
  } else if (LcurState == HIGH && RcurState == LOW || LcurState == LOW && RcurState == HIGH) {
    Volume -= 1;
  }
  //LprevState = LcurState;
  //RprevState = RcurState;
}





void volcheck() {
  if (Volume < 0) {
    Volume = 0;
  } else if (Volume > 50) {
    Volume = 50;
  }
}



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
  attachInterrupt(digitalPinToInterrupt(32), volume, CHANGE);
  pinMode(25, INPUT);
  delay(2000);
  lcd.clear();
  lcd.print(kannaal[channelcounter]);
  lcd.setCursor(0, 1);
  lcd.print(song);
}

void loop() {
  if (afterVolInterrupt) {
    lcd.clear();
    lcd.print("Volume ");
    volcheck();
    lcd.print(Volume);
    delay(1000);
    lcd.clear();
    lcd.print(kannaal[channelcounter]);
    lcd.print(Volume);
    lcd.setCursor(0, 1);
    lcd.print(song);
    afterVolInterrupt = false;
  }
}

void volume() {
  //for (int i = 0 ; i < 100; i++) {
    volloop();
  //}
  afterVolInterrupt = true;
}



void volloop() {
  LcurState = digitalRead(32);
  RcurState = digitalRead(25);
  if (LcurState == HIGH && RcurState == HIGH && LprevState == HIGH && RprevState == LOW) {
    Volume -= 2;
  } else if (LcurState == HIGH && RcurState == HIGH && LprevState == LOW && RprevState == HIGH) {
    Volume += 2;
  } else {
  }
  LprevState = LcurState;
  RprevState = RcurState;
}




void volcheck() {
  if (Volume < 0) {
    Volume = 0;
  } else if (Volume > 50) {
    Volume = 50;
  }
}

#include <Arduino.h>
#include <Wire.h>
#include <receiver.cpp>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  receiver mReceiver(0x64, 19);
  if (mReceiver.sendPatch() == 0) {
      mReceiver.loadFlash();
      mReceiver.boot();
      mReceiver.getPartInfo();
      mReceiver.getBootStatus();
  } else {
    Serial.println("Kon geen verbinding maken met DAB-chip");
  }
}

void loop() {
    Serial.write("Done");
    delay(100000);
}

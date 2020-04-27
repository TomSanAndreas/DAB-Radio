#include <Arduino.h>
#include <Wire.h>
#include <Receiver.cpp>
#include <RCHandler.cpp>
//#include <Radio.cpp>

BluetoothSerial btMonitor;
RCHandler mRCHandler;
Radio mRadio;

void setup() {
    Wire.begin();
    Serial.begin(9600);
    btMonitor.begin("DAB_RADIO");
    mRCHandler.subscribeTo(&btMonitor);
    mRCHandler.setRadio(&mRadio);
    Receiver mReceiver(0x64, 19);
    if (mReceiver.sendPatch() == 0) {
        mReceiver.loadFlash();
        mReceiver.boot();
        //mReceiver.getPartInfo();
        //mReceiver.getBootStatus();
    } else {
      Serial.println("Kon geen verbinding maken met DAB-chip");
    }
}

void loop() {
    mRCHandler.update();
    delay(500);
}

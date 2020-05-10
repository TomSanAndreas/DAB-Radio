#include <Arduino.h>
// #include <Wire.h>
// #include <Receiver.cpp>
#include <Radio.cpp>

BluetoothSerial btMonitor;
// RCHandler mRCHandler;
Radio mRadio;
Receiver mReceiver(0x64, 19);

void setup() {
    Wire.begin();
    Serial.begin(9600);
    btMonitor.begin("DAB_RADIO");
    mRadio.setBtMonitor(&btMonitor);
    // mRCHandler.setRadio(&mRadio);
    if (mReceiver.init()) {
        mReceiver.sendPatch();
        mReceiver.loadFlash();
        mReceiver.boot();
        //mReceiver.getBootStatus();
    } else {
      Serial.println("Kon geen verbinding maken met DAB-chip");
    }
    mRadio.setReceiver(&mReceiver);
    // mRadio.setServiceList(mReceiver.getServiceList(), mReceiver.totalAvailableServices, mReceiver.discoveredServices);
    mReceiver.startDigitalService(2);
}

void loop() {
    mRadio.update();
    delay(50);
}

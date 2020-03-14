#include <Arduino.h>
#include <Wire.h>
#include <si46xx.h>
#include <rom00_patch_016.h>

class receiver {
    private:
        int addr;
        int readStatus(int, bool);
        int wait(int, bool);
    public:
        receiver(int addr, int resetpin);
        int sendPatch();
        void loadFlash();
        void boot();
        bool getBootStatus();
        void getPartInfo();
};

receiver::receiver(int addr, int resetpin) {
    this->addr = addr;
    uint8_t bootSequence[16]; // was 16
    bootSequence[0] = SI46XX_POWER_UP; // zie pagina 155 van de programming guide
    bootSequence[1] = 0x00; // CTSIEN op 0 -> toggling van host interrupt line ligt af
    bootSequence[2] = (1 << 4) | (7 << 0); // clock mode op 0x01 -> referentie klok generator is in crystal mode | TR_SIZE is 0x7
    bootSequence[3] = 0x48; // IBIAS = 0x48
    bootSequence[4] = 0x00; // ARG 4 t.e.m. ARG7 dienen om crystal freq te regelen
    bootSequence[5] = 0xF9;
    bootSequence[6] = 0x24;
    bootSequence[7] = 0x01;
    bootSequence[8] = 0x1F; // CTUN = 0x1F
    bootSequence[9] = 0x00 | (1 << 4); // controlebyte, een 1 op bit 4
    bootSequence[10] = 0x00; // dient 0 te zijn
    bootSequence[11] = 0x00; // dient 0 te zijn
    bootSequence[12] = 0x00; // dient 0 te zijn
    bootSequence[13] = 0x00; // IBIAS_RUN = 0
    bootSequence[14] = 0x00; // dient 0 te zijn
    bootSequence[15] = 0x00; // dient 0 te zijn

    Serial.println("[BOOT] Beginnen met booten in 1s");
    pinMode(resetpin, OUTPUT);
    digitalWrite(resetpin, HIGH);
    delay(1000);
    Serial.println("[BOOT] Resetting...");
    digitalWrite(resetpin, LOW);
    delay(1000);

    Serial.println("[DEBUG] Controlleren van apparaat status");
    digitalWrite(resetpin, HIGH);
    delay(1000);

    wait(4, true);
    //getBootStatus();

    Serial.println("[BOOT] Sturen van de boot sequence");
    Wire.beginTransmission(addr);
    Wire.write(bootSequence, 16); // was 16
    if (!Wire.endTransmission()) { // 0 bij succes, anders wordt de if niet doorlopen
        //delay(50);
        //this->wait(4, true);
        delayMicroseconds(20);
        readStatus(4, true);
    }
    else {
        Serial.print("Geen i²c component gevonden op adres ");
        Serial.println(addr, HEX);
    }
}

int receiver::sendPatch() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_LOAD_INIT; // start LOAD_INIT commando
    sequence[1] = 0; // moet 0 zijn
    Wire.beginTransmission(addr);
    Wire.write(sequence, 2);
    if (!Wire.endTransmission()) {
        //this->wait(4, true);
        //delayMicroseconds(20);
        readStatus(4, true);
        float bytesSent = 0.0; // procentuele weergave -> float
        int numberOfBytes = firmware_rom00_patch_016_bin_len;
        int initialPatchBufferSize = 2048 + 4;  // aantal bytes plus de command header size
        uint8_t patchBuffer[initialPatchBufferSize];
        uint8_t *currentByte = (uint8_t *) firmware_rom00_patch_016_bin;

        patchBuffer[0] = SI46XX_HOST_LOAD; // begin van een stuk data
        patchBuffer[1] = 0x00; // moeten leeg zijn
        patchBuffer[2] = 0x00;
        patchBuffer[3] = 0x00;
        /*
        for (int i = 0; i < numberOfBytes; i++) {
            patchBuffer[i+4] = *currentByte++;
        }
        */
        Serial.println("[BOOT] Sending patch...");
        for (int i = 0; i < numberOfBytes / 2048; i++) { // data versturen in blokken van 2048
            for (int j = 0; j < 2048; j++) {
                patchBuffer[j+4] = *currentByte++;
                ++bytesSent;
            }
            Wire.beginTransmission(addr);
            Wire.write(patchBuffer, initialPatchBufferSize);
            Wire.endTransmission();
            Serial.print("[BOOT] Sending patch: ");
            Serial.print(bytesSent/numberOfBytes*100);
            Serial.println("% sent.");
            readStatus(4, true);
            //delayMicroseconds(20);
            //delay(1);
        }
        for (int j = 0; j < numberOfBytes % 2048; j++) {
            patchBuffer[j+4] = *currentByte++;
            ++bytesSent;
        }
        Wire.beginTransmission(addr);
        Wire.write(patchBuffer, numberOfBytes % 2048 + 4);
        Wire.endTransmission();
        Serial.print("[BOOT] Sending patch: ");
        Serial.print(bytesSent/numberOfBytes*100);
        Serial.println("% sent.");
        delay(4);
        //this->wait(4, true);
        readStatus(4, true);
        return 0;
    } else {
        Serial.print("[DEBUG] Geen i²c component gevonden op adres ");
        Serial.println(addr, HEX);
        return -1;
    }
}

void receiver::loadFlash() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_LOAD_INIT;
    sequence[1] = 0x00;
    Wire.beginTransmission(addr);
    Wire.write(sequence, 2);
    Wire.endTransmission();
    delay(50);
    this->wait(4, true);
    uint8_t flashSequence[12];
    flashSequence[0] = SI46XX_FLASH_LOAD;
    flashSequence[1] = 0x00;
    flashSequence[2] = 0x00;
    flashSequence[3] = 0x00;
    for (int i = 0; i < 4; i++) {
        flashSequence[7-i] = (addr_DAB >> 8*i) & 0xFF;
        Serial.print((addr_DAB >> 8*i) & 0xFF, BIN);
    }
    flashSequence[8] = 0x00;
    flashSequence[9] = 0x00;
    flashSequence[10] = 0x00;
    flashSequence[11] = 0x00;
    Wire.beginTransmission(addr);
    Wire.write(flashSequence, 12);
    Serial.println("[BOOT] Doorgeven van het DAB-flash adres");
    Wire.endTransmission();
    Serial.println("[BOOT] Starten met het inlezen van het DAB-flash adres");
    delay(5000);
    //this->wait(4, true);
    readStatus(4, true);
}

bool receiver::getBootStatus() {
    uint8_t sequence[6];
    Serial.println("[DEBUG] Boot status controlleren...");
	sequence[0] = SI46XX_GET_SYS_STATE;
	sequence[1] = 0x00;
    Wire.beginTransmission(addr);
	Wire.write(sequence, 2);
    Wire.endTransmission();
    delay(50);
    //delay(1000);
    //Wire.beginTransmission(addr);
    //sequence[0] = SI46XX_RD_REPLY;
    //Wire.write(sequence, 2);
    //Wire.endTransmission();
	readStatus(6, true);
    switch (Wire.read()) {
	case 0:
		Serial.println("[DEBUG] Bootloader is active");
		break;
	case 1:
		Serial.println("[DEBUG] FMHD is active");
		break;
	case 2:
		Serial.println("[DEBUG] DAB is active");
		return true;
	case 3:
		Serial.println("[DEBUG] TDMB or data only DAB image is active");
		break;
	case 4:
		Serial.println("[DEBUG] FMHD is active");
		break;
	case 5:
		Serial.println("[DEBUG] AMHD is active");
		break;
	case 6:
		Serial.println("[DEBUG] AMHD Demod is active");
		break;
	default:
		break;
	}
    Wire.read(); // laatste don't care uit buffer halen
    return false;
}

void receiver::getPartInfo() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_GET_PART_INFO;
    sequence[1] = 0x00;
    Wire.beginTransmission(addr);
    Wire.write(sequence, 2);
    Wire.endTransmission();
    delay(50);
    readStatus(23, true);
    uint8_t RESP4 = Wire.read();
    uint8_t RESP5 = Wire.read();
    Wire.read(); // RESP 6
    Wire.read(); // RESP 7
    uint16_t RESP8 = Wire.read();
    uint16_t RESP9 = Wire.read();
    for (uint8_t i = 10; i < 24; i++) {Wire.read();} // RESP10 t.e.m. RESP22
    Serial.print("[DEBUG] Response byte 4: ");
    Serial.println(RESP4, BIN);
    Serial.print("[DEBUG] Response byte 5: ");
    Serial.println(RESP5, BIN);
    Serial.print("[DEBUG] Response byte 9 & 8: ");
    Serial.print(RESP9, BIN);
    Serial.println(RESP8, BIN);
    Serial.print("[DEBUG] Decimal (= part number): ");
    uint16_t partnumber = (RESP9 << 8) | RESP8;
    Serial.println(partnumber);
}

void receiver::boot() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_BOOT;
    sequence[1] = 0x00;
    Wire.write(sequence, 2);
    Serial.println("[BOOT] DAB-Receiver opstarten...");
    Wire.endTransmission();
    delay(300);
    this->wait(4, true);
}

int receiver::wait(int nStatusBytes, bool checkForError) {
    Wire.beginTransmission(addr);
    Wire.write(SI46XX_RD_REPLY);
    delay(50); // eerste vertraging
    int iteratie = 0;
    int status;
    if (!Wire.endTransmission()) { // controlleren of er een i²c component op dit adres zit
        status = readStatus(nStatusBytes, checkForError);
        while (status == -1) {
            ++iteratie;
            if (checkForError) {
                Serial.print("[DEBUG] CTS was 0, wachten met verdergaan (");
                Serial.print(iteratie);
                Serial.println(")");
            }
            delay(250);
            Wire.beginTransmission(addr);
            Wire.write(SI46XX_RD_REPLY);
            Wire.endTransmission();
            status = readStatus(nStatusBytes, checkForError);
        }
        return status;
    } else {
        Serial.print("Geen i²c component gevonden op adres ");
        Serial.println(addr, HEX);
        return -1;
    }
}

int receiver::readStatus(int nStatusBytes, bool CheckForErrors) {
    delay(50); // eerste vertraging
    Wire.requestFrom(addr, nStatusBytes); // n bytes antwoord verwacht
    while (Wire.available() < 4) {} // wachten tot er 4 bytes aan data beschikbaar zijn
    int status0 = Wire.read(); // eerste byte lezen
    int status1 = Wire.read();
    int status2 = Wire.read();
    int status3 = Wire.read();
    bool gebruikAndereStatuscodes = false;
    Serial.print("[DEBUG] Status byte 0: ");
    Serial.println(status0, BIN);
    if (status0 & 0x80) { // bit 7 is CTS, bit 7 = 1 -> klaar voor volgend commando
        Serial.println("[DEBUG] Clear to send = 1");
        Serial.println("[DEBUG] > Het is veilig om de volgende command te sturen");
    } else {
        Serial.println("[DEBUG] Clear to send = 0");
        Serial.println("[DEBUG] Andere status bytes:");
        Serial.print("[DEBUG] Status byte 1: ");
        Serial.println(status1, BIN); // byte 2
        Serial.print("[DEBUG] Status byte 2: ");
        Serial.println(status2, BIN); // byte 3
        Serial.print("[DEBUG] Status byte 3: ");
        Serial.println(status3, BIN); // byte 4
        return -1; // indicatie om te wachten met sturen van volgende command
    }
    if (CheckForErrors) {
        if (status0 & 0x40) { // bit 6 is ERR_CMD, bit 6 = 1 -> error
            Serial.print("[DEBUG] Statuscode is ");
            Serial.println(status0, BIN);
            Serial.println("[DEBUG] Er liep iets fout bij het sturen van het commando");
            gebruikAndereStatuscodes = true;
        } else {
            Serial.println("[DEBUG] Commando werd correct ontvangen!");
        }
        if (gebruikAndereStatuscodes) {
            Serial.println("[DEBUG] Foutcodes van het commando: ");
            Serial.print("[DEBUG] Code 1: ");
            Serial.println(status1, BIN);
            Serial.print("[DEBUG] Code 2: ");
            Serial.println(status2, BIN);
        }
        Serial.print("[DEBUG] 3e status byte patroon: ");
        Serial.println(status3, BIN);
        if (status3 & 0x0C) { // bit 2 en 3, dient als aanwijzing voor een te hoge SPI clock rate
            Serial.println("[DEBUG] SPI Clock rate probleem");
        }
        if (status3 & 0x02) {
            Serial.println("[DEBUG] Arbiter error");
        }
        if (status3 & 0x01) {
            Serial.println("[DEBUG] System keep alive timer is verlopen, non-recoverable error");
        }
        switch (status3 << 6) { // bit 6 & 7, codes van 0 (0b00) tem 3 (0b11)
            case 0: { // = systeem is gereset, wachten op power up
                Serial.println("[DEBUG] Systeem is gereset, wachten op powerup commando");
                return 0;
            };
            case 1: {
                Serial.println("[DEBUG] Reserved");
                return 1;
            };
            case 2: {
                Serial.println("[DEBUG] De bootloader is bezig");
                return 2;
            };
            case 3: {
                Serial.println("[DEBUG] De applicatie verloopt correct");
                return 3;
            };
        }
    } else {
        return 3;
    }
    return 4;
}
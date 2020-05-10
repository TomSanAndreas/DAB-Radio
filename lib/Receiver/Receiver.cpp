#include <Types.cpp>
#include <Constants.cpp>
#include <Wire.h>
#include <si46xx.h>
#include <rom00_patch_016.h>

class Receiver {
    private:
        int addr;
        int resetpin;
        int nServices = 0; // aantal beschikbare services (nog niet aanspreekbaar, maar wel detecteerbaar)
        uint8_t currentlyTunedFrequency;

        int readStatus(int, bool);
        void waitForCTS();

        void tuneInto(uint8_t); // tunen naar een bepaalde frequentie
        void waitForTuningComplete();

        void setProperties();
        void setProperty(uint16_t, uint16_t);

        void initDab();
        void scan(uint8_t = 3); // leeghalen van de services lijst met meegegeven index en opnieuw opvullen met opgevangen signalen, 3 is alles opnieuw scannen
    public:
    // aantal beschikbare services
        Service** services = (Service**) malloc(3*sizeof(Service*)); // 3 frequenties bekend voor België, pointer naar de eerste frequency, naar de eerste service
        uint8_t discoveredServices[3]; // aantal services beschikbaar bij de DAB (aanspreekbaar/kan naar getuned worden) per frequentie
        int totalAvailableServices = 0;
        uint32_t currentlyActiveServiceId = 0;

        bool serviceActive = false;
    // functies om de DAB-chip te doen opstarten:
        Receiver(uint8_t, uint8_t);
        bool init();
        int sendPatch();
        void loadFlash();
        void boot();

    // functies voor een opgestarte DAB-chip te besturen:
        void scanAll();
        Service** getServiceList();
        void startDigitalService(uint8_t, uint8_t = 0, uint8_t = 0); // serviceIndex (in de lijst bekende services), frequentie lijst index (0-2), componentIndex
        void startServiceWithID(uint32_t);
        void stopDigitalService();
        void setVolume(uint8_t);
        void mute();
        void unmute();

    // diagnostische functies
        bool getBootStatus();
        void getPartInfo();
};

Receiver::Receiver(uint8_t addr, uint8_t rstpin) {
    this->addr = addr;
    this->resetpin = rstpin;
    services[0] = nullptr;
    services[1] = nullptr;
    services[2] = nullptr;
}

void Receiver::scanAll() {
    scan();
}

Service** Receiver::getServiceList() {
    return services;
}

bool Receiver::init() {
    uint8_t bootSequence[16];
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
    Serial.println("[BOOT] Resetting...");
    digitalWrite(resetpin, LOW);
    delay(100);
    Serial.println("[DEBUG] Controlleren van apparaat status");
    digitalWrite(resetpin, HIGH);
    delay(1000);
    waitForCTS();
    Serial.println("[BOOT] Sturen van de boot sequence");
    Wire.beginTransmission(addr);
    Wire.write(bootSequence, 16);
    if (!Wire.endTransmission()) { // 0 bij succes, anders wordt de if niet doorlopen
        waitForCTS();
        return true;
    }
    else {
        Serial.print("Geen i²c component gevonden op adres ");
        Serial.println(addr, HEX);
        return false;
    }
}

int Receiver::sendPatch() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_LOAD_INIT; // start LOAD_INIT commando
    sequence[1] = 0; // moet 0 zijn
    Wire.beginTransmission(addr);
    Wire.write(sequence, 2);
    if (!Wire.endTransmission()) {
        Serial.println("[BOOT] Apparaat voorbereiden om de patch te verzenden");
        waitForCTS();
        float bytesSent = 0.0; // procentuele weergave -> float
        int numberOfBytes = firmware_rom00_patch_016_bin_len;
        int initialPatchBufferSize = 64 + 4;  // aantal bytes plus de command header size
        uint8_t patchBuffer[initialPatchBufferSize];
        uint8_t *currentByte = (uint8_t *) firmware_rom00_patch_016_bin;

        patchBuffer[0] = SI46XX_HOST_LOAD; // begin van een stuk data
        patchBuffer[1] = 0x00; // moeten leeg zijn
        patchBuffer[2] = 0x00;
        patchBuffer[3] = 0x00;
        Serial.print("[BOOT] Sending patch...");
        for (int i = 0; i < numberOfBytes / 64; i++) { // data versturen in blokken van 2048
            for (int j = 0; j < 64; j++) {
                patchBuffer[j+4] = *currentByte++;
                ++bytesSent;
            }
            Wire.beginTransmission(addr);
            Wire.write(patchBuffer, initialPatchBufferSize);
            Wire.endTransmission();
            Serial.print(bytesSent/numberOfBytes*100);
            Serial.print("%\t");
            delay(4); // 50 ms tussen verschillende stukken patch zodat er op de CTS kan worden gewacht
            waitForCTS();
        }
        for (int j = 0; j < numberOfBytes % 64; j++) {
            patchBuffer[j+4] = *currentByte++;
            ++bytesSent;
        }
        Wire.beginTransmission(addr);
        Wire.write(patchBuffer, numberOfBytes % 64 + 4);
        Wire.endTransmission();
        Serial.print("\n[BOOT] Sending patch: ");
        Serial.print(bytesSent/numberOfBytes*100);
        Serial.println("% sent.");
        delay(4);
        waitForCTS();
        return 0;
    } else {
        Serial.print("[DEBUG] Geen i²c component gevonden op adres ");
        Serial.println(addr, HEX);
        return -1;
    }
}

void Receiver::loadFlash() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_LOAD_INIT;
    sequence[1] = 0x00;
    Wire.beginTransmission(addr);
    Wire.write(sequence, 2);
    Wire.endTransmission();
    delay(1);
    waitForCTS();
    uint8_t flashSequence[12];
    flashSequence[0] = SI46XX_FLASH_LOAD;
    flashSequence[1] = 0x00;
    flashSequence[2] = 0x00;
    flashSequence[3] = 0x00;
    for (int i = 0; i < 4; i++) {
        flashSequence[4+i] = (addr_DAB >> 8*i) & 0xFF;
    }
    flashSequence[8] = 0x00;
    flashSequence[9] = 0x00;
    flashSequence[10] = 0x00;
    flashSequence[11] = 0x00;
    Wire.beginTransmission(addr);
    Wire.write(flashSequence, 12);
    Serial.println("[BOOT] Doorgeven van het DAB-flash adres");
    Wire.endTransmission();
    delay(1000);
    waitForCTS();
}

void Receiver::boot() {
    uint8_t sequence[2];
    sequence[0] = SI46XX_BOOT;
    sequence[1] = 0x00;
    Wire.beginTransmission(addr);
    Wire.write(sequence, 2);
    Serial.println("[BOOT] DAB-Receiver opstarten...");
    Wire.endTransmission();
    delay(300);
    waitForCTS();

    setProperties();

    waitForCTS();
    getBootStatus();
    waitForCTS();
    
    initDab();
}

bool Receiver::getBootStatus() {
    uint8_t sequence[6];
    Serial.println("[DEBUG] Boot status controlleren...");
	sequence[0] = SI46XX_GET_SYS_STATE;
	sequence[1] = 0x00;
    Wire.beginTransmission(addr);
	Wire.write(sequence, 2);
    Wire.endTransmission();
    delay(50);
	readStatus(6, false);
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

void Receiver::getPartInfo() {
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

void Receiver::setVolume(uint8_t volume) {
    uint16_t vol = 0x0000;
    vol |= volume;
    unmute();
    setProperty(SI46XX_AUDIO_ANALOG_VOLUME, vol);
}

void Receiver::mute() {
    setProperty(SI46XX_AUDIO_MUTE, 0x0003);
}

void Receiver::unmute() {
    setProperty(SI46XX_AUDIO_MUTE, 0x0000);
}

void Receiver::setProperties() {
    setProperty(SI46XX_DAB_TUNE_FE_CFG, 0x0001);
    setProperty(SI46XX_DAB_TUNE_FE_VARM, 0x1710);
    setProperty(SI46XX_DAB_TUNE_FE_VARB, 0x1711);
    setProperty(SI46XX_PIN_CONFIG_ENABLE, 0x0001);
    Serial.println("[DEBUG] Properties zijn ingesteld!");
}

void Receiver::setProperty(uint16_t type, uint16_t value) {
    uint8_t sequence[6];
    sequence[0] = SI46XX_SET_PROPERTY;
    sequence[1] = 0;
    sequence[2] = type & 0xFF;
    sequence[3] = (type >> 8) & 0xFF;
    sequence[4] = value & 0xFF;
    sequence[5] = (value >> 8) & 0xFF;
    Wire.beginTransmission(addr);
    Wire.write(sequence, 6);
    Wire.endTransmission();
    waitForCTS();
}

void Receiver::initDab() {
    // instellen van DAB freq lijst
    uint8_t sequence[16];
    uint16_t len = 0;
    sequence[0] = SI46XX_DAB_SET_FREQ_LIST;
    sequence[1] = 3;
    sequence[2] = 0;
    sequence[3] = 0;
    sequence[4] = CHAN_12A & 0xFF; // eerste frequentie laag binnen België
    sequence[5] = (CHAN_12A >> 8) & 0xFF;
    sequence[6] = (CHAN_12A >> 16) & 0xFF;
    sequence[7] = (CHAN_12A >> 24) & 0xFF;
    sequence[8] = CHAN_11A & 0xFF; // tweede frequentie laag
    sequence[9] = (CHAN_11A >> 8) & 0xFF;
    sequence[10] = (CHAN_11A >> 16) & 0xFF;
    sequence[11] = (CHAN_11A >> 24) & 0xFF;
    sequence[12] = CHAN_12B & 0xFF; // derde frequentie laag
    sequence[13] = (CHAN_12B >> 8) & 0xFF;
    sequence[14] = (CHAN_12B >> 16) & 0xFF;
    sequence[15] = (CHAN_12B >> 24) & 0xFF;
    Wire.beginTransmission(addr);
    Wire.write(sequence, 16);
    Wire.endTransmission();
    delay(50);
    waitForCTS();
    // eerste station selecteren
    tuneInto(0);
    Serial.println("[DEBUG] Klaar met tunen naar de eerste zender!");

    sequence[0] = SI46XX_DAB_GET_DIGITAL_SERVICE_LIST;
    sequence[1] = 0;
    while (len < 6) {
        delay(150);
        Wire.beginTransmission(addr);
        Wire.write(sequence, 2);
        Wire.endTransmission();
        delay(50);
        Wire.requestFrom(addr, 6);
        Wire.read(); // niet belangrijk
        Wire.read(); // niet belangrijk
        Wire.read(); // niet belangrijk
        Wire.read(); // niet belangrijk
        // uint16_t len = Wire.read(); // bevat de laagste 8 bits vd lengte
        uint8_t resp1 = Wire.read();
        uint8_t resp2 = Wire.read();
        len = resp1;
        len |= (resp2 << 8); // bevat de hoogste 8 bits vd lengte
        if (len > 6) {
            break;
        } else {
            Serial.println("[DEBUG] Er zijn te weinig gegevens beschikbaar");
            waitForCTS();
        }
    }
    // Serial.print("[DEBUG] De lengte van de service lijst is ");
    // Serial.println(len);
    // delay(150);

    delay(1000);
    waitForCTS();
    scan();
    // Wire.beginTransmission(addr);
    // Wire.write(sequence, 2);
    // Wire.endTransmission();
    // delay(50);
    // // uint8_t buffer[len + 6];
    // Wire.requestFrom(addr, len + 6); // de rest van de lijst ophalen, volm opschreven op p421 vd programming guide
    // // Wire.readBytes(buffer, len + 6);

    // // for (uint16_t i = 0; i < len + 6; i++) {
    // //     Serial.println(buffer[i], HEX);
    // // }
    // Wire.read(); Wire.read(); Wire.read(); Wire.read(); // 4 bytes header met CTS enz
    // Wire.read(); Wire.read(); // lijst lengte, er wordt verondersteld dat deze niet gewijzigd is

    // Wire.read(); // bevat de lijst versie, wordt momenteel geen rekening mee gehouden
    // Wire.read(); // 2e byte van de lijst versie
    // nServices = Wire.read(); // aantal services die in de lijst te lezen zijn
    // // uint8_t nServices = buffer[8]; // 2 reads, +6 wegens de voorgaande informatie die de lengte en de originele header bevat
    // Serial.print("[DEBUG] Er zijn ");
    // Serial.print(nServices);
    // Serial.println(" services gevonden!");
    // Wire.read(); Wire.read(); Wire.read(); // 3 'reserved for future use' bytes
    // services = (Service**) malloc(3*sizeof(Service*)); // 3 frequenties bekend voor België
    // services[0] = (Service*) malloc(nServices * sizeof(Service));
    // for (uint8_t j = 0; j < nServices; j++) {
    //     Service currentService;
    //     // Wire.read(); Wire.read(); Wire.read(); Wire.read(); // serviceID, wordt nu gebruikt hieronder
    //     currentService.service_id = Wire.read(); // service ID eerste byte
    //     currentService.service_id |= Wire.read() << 8; // service ID tweede byte
    //     currentService.service_id |= Wire.read() << 16; // service ID derde byte
    //     currentService.service_id |= Wire.read() << 24; // service ID vierde byte
    //     Wire.read(); // bevat overige informatie zoals service type die nu niet wordt gebruikt
    //     currentService.nComponenten = Wire.read() & 0x0F; // bevat nog meer informatie die niet wordt gebruikt, enkel het aantal componenten per service is belangrijk
    //     currentService.componenten = (Component*) malloc(currentService.nComponenten * sizeof(Component));
    //     Wire.read(); // bevat informatie rond charset, wordt nu nog genegeerd
    //     Wire.read(); // align pad (bevat geen informatie)
    //     // char serviceName[17];
    //     for (uint8_t i = 0; i < 16; i++) {
    //         uint8_t currentChar = Wire.read();
    //         if (currentChar != 0xFF) {
    //             currentService.service_label[i] = currentChar;
    //         } else {
    //             currentService.service_label[i] = '\0';
    //             break;
    //         }
    //         // Serial.print(" 0x");
    //         // Serial.print(currentChar, HEX);
    //     }
    //     // Serial.println("\n---");
    //     currentService.service_label[16] = '\0'; // string beëindigen
    //     // serviceNames[j] = serviceName;
    //     // Serial.println(serviceName);
    //     for (uint8_t i = 0; i < currentService.nComponenten; i++) {
    //         //Wire.read(); Wire.read(); // 2 bytes voor component ID, wordt nu niet gebruikt
    //         currentService.componenten[i].component_id = Wire.read(); // eerste byte van component ID
    //         currentService.componenten[i].component_id |= Wire.read() << 8; // tweede byte van component ID
    //         Wire.read(); // component info, wordt nu niet gebruikt
    //         Wire.read(); // extra vlaggen, wordt nu niet gebruikt
    //     }
    //     services[0][j] = currentService;
    // }
    // voor debugging, uitprinten van huidige gegevens
    // discoveredServices[0] = 0;
    // uint32_t unknownServiceId = 0xFFFFFFFF;
    // for (uint8_t i = 0; i < nServices; i++) {
    //     if (services[0][i].service_id != unknownServiceId) {
    //         Serial.println(services[0][i].service_label);
    //         Serial.println(services[0][i].service_id, HEX);
    //         Serial.println("Componenten:");
    //         for (uint8_t j = 0; j < services[0][i].nComponenten; j++) {
    //             Serial.println(services[0][i].componenten[i].component_id, HEX);
    //         }
    //         Serial.println("---");
    //         discoveredServices[0]++;
    //     }
    // }

    // vanaf nu zijn de gegevens binnenin de receiver class klaar om over bluetooth te sturen
    // digitale service automatisch starten
    // waitForCTS();
    // for (uint8_t i = 0; i < discoveredServices; i++) {
    //     startDigitalService(i);
    //     delay(10000);
    // }
    //startDigitalService(0); // automatisch de eerste service starten
}

void Receiver::scan(uint8_t frequencyIndex) {
    if (frequencyIndex >= 3) {
        totalAvailableServices = 0;
        for (uint8_t i = 0; i < 3; i++) {
            discoveredServices[i] = 0;
            tuneInto(i);
            // delay(1000); // ?
            uint8_t timeout = 3;
            uint16_t len = 0;
            uint8_t sequence[2];
            sequence[0] = SI46XX_DAB_GET_DIGITAL_SERVICE_LIST;
            sequence[1] = 0;
            while (len == 0 && timeout) {
                Wire.beginTransmission(addr);
                Wire.write(sequence, 2);
                Wire.endTransmission();
                delay(750); // WAS : 50
                Wire.requestFrom(addr, 6);
                Wire.read(); // niet belangrijk
                Wire.read(); // niet belangrijk
                Wire.read(); // niet belangrijk
                Wire.read(); // niet belangrijk
                len = Wire.read();
                len |= Wire.read() << 8;
                // Serial.print("Lengte: ");
                // Serial.println(len);
                waitForCTS();
                --timeout;
                delay(250);
            }
            if (!timeout) {
                Serial.print("[ERROR] Kon geen informatie van de ");
                Serial.print(i + 1);
                Serial.println("e frequentie inlezen");
                continue;
            }

            Wire.beginTransmission(addr);
            Wire.write(sequence, 2);
            Wire.endTransmission();
            delay(750); // WAS : 50
            Wire.requestFrom(addr, len + 6); // de rest van de lijst ophalen, vorm opschreven op p421 vd programming guide
            Wire.read(); Wire.read(); Wire.read(); Wire.read(); // 4 bytes header met CTS enz
            
            Wire.read(); Wire.read(); // lijst lengte, er wordt verondersteld dat deze niet gewijzigd is
            
            Wire.read(); // bevat de lijst versie, wordt momenteel geen rekening mee gehouden
            Wire.read(); // 2e byte van de lijst versie
            nServices = Wire.read(); // aantal services die in de lijst te lezen zijn
            Wire.read(); Wire.read(); Wire.read(); // 3 'reserved for future use' bytes
            if (services[i] != nullptr) {
                //TODO: vrijgeven van geheugen namen van elk station
                free(services[i]);
            }
            services[i] = (Service*) malloc(nServices * sizeof(Service));
            for (uint8_t j = 0; j < nServices; j++) {
                Service currentService;
                currentService.service_id = Wire.read(); // service ID eerste byte
                currentService.service_id |= Wire.read() << 8; // service ID tweede byte
                currentService.service_id |= Wire.read() << 16; // service ID derde byte
                currentService.service_id |= Wire.read() << 24; // service ID vierde byte
                Wire.read(); // bevat overige informatie zoals service type die nu niet wordt gebruikt
                currentService.nComponenten = Wire.read() & 0x0F; // bevat nog meer informatie die niet wordt gebruikt, enkel het aantal componenten per service is belangrijk
                currentService.componenten = (Component*) malloc(currentService.nComponenten * sizeof(Component));
                Wire.read(); // bevat informatie rond charset, wordt nu nog genegeerd
                Wire.read(); // align pad (bevat geen informatie)
                for (uint8_t i = 0; i < 16; i++) {
                    uint8_t currentChar = Wire.read();
                    if (currentChar != 0xFF) {
                        currentService.service_label[i] = currentChar;
                        } else {
                        currentService.service_label[i] = '\0';
                        break;
                    }
                }
                currentService.service_label[16] = '\0'; // string beëindigen
                for (uint8_t k = 0; k < currentService.nComponenten; k++) {
                    currentService.componenten[k].component_id = Wire.read(); // eerste byte van component ID
                    currentService.componenten[k].component_id |= Wire.read() << 8; // tweede byte van component ID
                    Wire.read(); // component info, wordt nu niet gebruikt
                    Wire.read(); // extra vlaggen, wordt nu niet gebruikt
                }
                services[i][j] = currentService;
            }
            for (uint8_t j = 0; j < nServices; j++) {
                if (services[i][j].service_id != 0xFFFFFFFF) { // services die niet herkend zijn krijgen deze ID
                    discoveredServices[i]++;
                    totalAvailableServices++;
                }
            }
            delay(150);
        }
    }
    waitForCTS();
    // vanaf nu zijn de gegevens binnenin de receiver class klaar om over bluetooth te sturen

    // Alle gekende stations uitprinten
    // for (uint8_t i = 0; i < 3; i++) {
    //     Serial.print(i);
    //     Serial.print(" - ");
    //     Serial.print(discoveredServices[i]);
    //     Serial.println(" services");
    //     for (uint8_t j = 0; j < discoveredServices[i]; j++) {
    //         Serial.println(services[i][j].service_label);
    //         Serial.println(services[i][j].service_id, HEX);
    //         // Serial.println(services[i][j].componenten[0].component_id, HEX);
    //         Serial.println("---");
    //     }
    //     Serial.println("-");
    // }

    //startDigitalService(0); // automatisch de eerste service starten
}

void Receiver::tuneInto(uint8_t freqIndex) {
    uint8_t sequence[6];
    sequence[0] = SI46XX_DAB_TUNE_FREQ;
    sequence[1] = 0;
    sequence[2] = freqIndex; // eerste laag is 0, tweede laag is 1, derde laag is 2
    sequence[3] = 0;
    sequence[4] = 0; // antcap, 0 betekend automatisch
    sequence[5] = 0;

    Wire.beginTransmission(addr);
    Wire.write(sequence, 6);
    Wire.endTransmission();
    delay(50);
    waitForTuningComplete();
    waitForCTS();
    currentlyTunedFrequency = (freqIndex < 3) ? freqIndex : 0;
}

void Receiver::startServiceWithID(uint32_t serviceId) {
    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < discoveredServices[i]; j++) {
            if (services[i][j].service_id == serviceId) {
                startDigitalService(j, i);
                Serial.println(services[i][j].service_label);
                return; // klaar
            }
        }
    }
    currentlyTunedFrequency = 0;
    Serial.println("UNKNOWN!");
}

void Receiver::startDigitalService(uint8_t serviceIndex, uint8_t frequencyIndex, uint8_t componentIndex) {
    if (currentlyTunedFrequency != frequencyIndex) {
        tuneInto(frequencyIndex);
        delay(150); // voor directe werking te garanderen
    }
    uint8_t sequence[12];
    sequence[0] = SI46XX_DAB_START_DIGITAL_SERVICE;
    sequence[1] = 0;
    sequence[2] = 0;
    sequence[3] = 0;
    sequence[4] = services[currentlyTunedFrequency][serviceIndex].service_id & 0xFF;
    sequence[5] = (services[currentlyTunedFrequency][serviceIndex].service_id >> 8) & 0xFF;
    sequence[6] = (services[currentlyTunedFrequency][serviceIndex].service_id >> 16) & 0xFF;
    sequence[7] = (services[currentlyTunedFrequency][serviceIndex].service_id >> 24) & 0xFF;
    sequence[8] = services[currentlyTunedFrequency][serviceIndex].componenten[componentIndex].component_id & 0xFF;
    sequence[9] = (services[currentlyTunedFrequency][serviceIndex].componenten[componentIndex].component_id >> 8)& 0xFF;
    sequence[10] = (services[currentlyTunedFrequency][serviceIndex].componenten[componentIndex].component_id >> 16)& 0xFF;
    sequence[11] = (services[currentlyTunedFrequency][serviceIndex].componenten[componentIndex].component_id >> 24)& 0xFF;

    Wire.beginTransmission(addr);
    Wire.write(sequence, 12);
    Wire.endTransmission();
    Wire.requestFrom(addr, 4);
    for (int i = 0; i < 3; i++) {
        Wire.read();
    }
    if (Wire.read()) { // error code, 0 is succes
        serviceActive = false;
    } else {
        serviceActive = true;
    }
    delay(40);
    currentlyActiveServiceId = services[currentlyTunedFrequency][serviceIndex].service_id;
    waitForCTS();
}

void Receiver::stopDigitalService() {
    uint8_t sequence[12];
    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < discoveredServices[i]; j++) {
            if (services[i][j].service_id == currentlyActiveServiceId) {
                sequence[0] = SI46XX_DAB_STOP_DIGITAL_SERVICE;
                sequence[1] = 0;
                sequence[2] = 0;
                sequence[3] = 0;
                sequence[4] = services[i][j].service_id & 0xFF;
                sequence[5] = (services[i][j].service_id >> 8) & 0xFF;
                sequence[6] = (services[i][j].service_id >> 16) & 0xFF;
                sequence[7] = (services[i][j].service_id >> 24) & 0xFF;
                sequence[8] = services[i][j].componenten[0].component_id & 0xFF;
                sequence[9] = (services[i][j].componenten[0].component_id >> 8)& 0xFF;
                sequence[10] = (services[i][j].componenten[0].component_id >> 16)& 0xFF;
                sequence[11] = (services[i][j].componenten[0].component_id >> 24)& 0xFF;
                // sequence[8] = 0x00;
                // sequence[9] = 0x00;
                // sequence[10] = 0x00;
                // sequence[11] = 0x00;

                Wire.beginTransmission(addr);
                Wire.write(sequence, 12);
                Wire.endTransmission();
                waitForCTS();
                currentlyActiveServiceId = 0;
                serviceActive = false;
                return;
            }
        }
    }

}

void Receiver::waitForCTS() {
    bool ready = false;
    while (!ready) {
        Wire.beginTransmission(addr);
        Wire.write(SI46XX_RD_REPLY);
        Wire.endTransmission();
        Wire.requestFrom(addr, 4);
        uint8_t resultByte = Wire.read(); // bevat de CTS bit
        Wire.read(); // niet-gebruikte 2e byte
        Wire.read(); // niet-gebruikte 3e byte
        Wire.read(); // niet-gebruikte 4e byte
        ready = (0x80 & resultByte) == 0x80; // controleren of CTS gezet is
    }
}

void Receiver::waitForTuningComplete() {
    Serial.print("\n[DEBUG] Tunen");
    bool ready = false;
    while (!ready) {
        Wire.beginTransmission(addr);
        Wire.write(SI46XX_RD_REPLY);
        Wire.endTransmission();
        Wire.requestFrom(addr, 4);
        uint8_t resultByte = Wire.read(); // bevat de CTS en de STCINT bit
        Wire.read(); // niet-gebruikte 2e byte
        Wire.read(); // niet-gebruikte 3e byte
        Wire.read(); // niet-gebruikte 4e byte
        ready = (0x81 & resultByte) == 0x81; // controleren of CTS en STCINT gezet is
        // Serial.println(resultByte, BIN);
        Serial.print(".");
        delay(10);
    }
    Serial.println(" compleet!");
}

int Receiver::readStatus(int nStatusBytes, bool CheckForErrors) {
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
// #include <Constants.cpp>
// #include <Types.cpp>
#include <BluetoothSerial.h>
#include <Receiver.cpp>

class Radio {
    private:
        uint8_t volume = 63; // weergave zoals in de property van de DAB-chip
        bool muted = false;
        PendingCommand* currentPendingCommand = nullptr; // linked list logica
        PendingCommand* lastPendingCommand = nullptr;
        // Service** currentServiceList = nullptr;
        // uint8_t* nServicesPerFreq = nullptr;
        // uint8_t nServices = 0;
        Receiver* receiver = nullptr;

        BluetoothSerial* btMonitor = nullptr;
        bool activeBtConnection = false;

        PendingCommand* addCommand(uint8_t, uint16_t);

        void send(PendingCommand);

        void addServiceInformationCommand();
        void addCurrentVolumeCommand();
        void addCurrentActiveServiceCommand();
    public:
        bool hasPendingCommands = false;

        //voor init
        void setBtMonitor(BluetoothSerial* bt) {
            btMonitor = bt;
        }

        //functies
        void resendAllInformation();

        //setters
        void setVolume(uint8_t);
        void setReceiver(Receiver* r) {
            receiver = r;
        }
        // void setServiceList(Service**, uint8_t, uint8_t*);

        PendingCommand nextCommand();

        //bluetooth IO updaten
        void update();
};

void Radio::setVolume(uint8_t newVolume) {
    volume = newVolume;
    addCommand(SET_VOLUME, 1);
    *(lastPendingCommand->commandContent) = volume;
}

// void Radio::setServiceList(Service** newList, uint8_t nServ, uint8_t* nServPerFreq) {
//     currentServiceList = newList;
//     this->nServicesPerFreq = nServPerFreq;
//     this->nServices = nServ;
//     // addServiceInformationCommand();
// }

void Radio::resendAllInformation() {
    addServiceInformationCommand();
    addCurrentVolumeCommand();
    addCurrentActiveServiceCommand();
}

void Radio::addCurrentVolumeCommand() {
    addCommand(CURRENT_VOLUME, 1)->commandContent[0] = (muted)? 0 : volume;
}

void Radio::addCurrentActiveServiceCommand() {
    addCommand(CURRENT_SERVICE, 4);
    if (receiver->serviceActive) {
        lastPendingCommand->commandContent[0] = (receiver->currentlyActiveServiceId) >> 24;
        lastPendingCommand->commandContent[1] = (receiver->currentlyActiveServiceId) >> 16;
        lastPendingCommand->commandContent[2] = (receiver->currentlyActiveServiceId) >> 8;
        lastPendingCommand->commandContent[3] = (receiver->currentlyActiveServiceId);
    } else {
        lastPendingCommand->commandContent[0] = 0;
        lastPendingCommand->commandContent[1] = 0;
        lastPendingCommand->commandContent[2] = 0;
        lastPendingCommand->commandContent[3] = 0;
    }
}

void Radio::addServiceInformationCommand() {
    uint16_t cmdLength = 1 + (16 + 4) * receiver->totalAvailableServices; // 17 bytes per service naam, 4 bytes per service ID, 1 eerste byte voor het aantal services
    addCommand(SERVICE_LIST, cmdLength); 
    uint8_t* cmdContent = lastPendingCommand->commandContent;
    *cmdContent++ = receiver->totalAvailableServices; // aantal services meegeven als eerste deel van het commando
    // Serial.print("Totaal aantal services beschikbaar: ");
    // Serial.println(nServices);
    for (uint8_t i = 0; i < 3; i++) {
        // Serial.print("Aantal services in het ");
        // Serial.print(i + 1);
        // Serial.print("e frequentiegebied: ");
        // Serial.println(nServicesPerFreq[i]);
        for (uint8_t j = 0; j < receiver->discoveredServices[i]; j++) { // max 15 services per frequentie
            if (strlen(receiver->services[i][j].service_label)) {
                char* service_label_char = receiver->services[i][j].service_label;
                while (*service_label_char) {
                    *cmdContent++ = *service_label_char++;
                }
                *cmdContent++ = (receiver->services[i][j].service_id >> 24) & 0xFF;
                *cmdContent++ = (receiver->services[i][j].service_id >> 16) & 0xFF;
                *cmdContent++ = (receiver->services[i][j].service_id >> 8) & 0xFF;
                *cmdContent++ = receiver->services[i][j].service_id & 0xFF;
            }
        }
    }

    // voor debugging
    // uint8_t* p = lastPendingCommand->commandContent;
    // p++; // aantal services
    // Serial.println();
    // // commando inhoud bevat spaties tussen de zenders zodat elke zender 16 tekens is
    // for (uint8_t i = 0; i < nServices; i++) {
    //     for (uint8_t j = 0; j < 16; j++) {
    //         Serial.print((char) *p++);
    //     }
    //     for (uint8_t j = 0; j < 4; j++) {
    //         Serial.print(*p++, HEX);
    //     }
    // }
}

void Radio::update() {
    if (btMonitor && receiver) {
        while (btMonitor->available()) {
            uint8_t instruction = btMonitor->read();
            switch (instruction) {
                case CONNECT:
                    activeBtConnection = true;
                    Serial.println("[BLUETOOTH] A device has been connected");
                    if (receiver->totalAvailableServices != 0) {
                        resendAllInformation();
                    }
                    break;
                case DISCONNECT:
                    activeBtConnection = false;
                    Serial.println("[BLUETOOTH] Device disconnected");
                    break;
                // case REQUEST_VOLUME:
                //     Serial.println("[BLUETOOTH] Sending current volume");
                //     addCommand(CURRENT_VOLUME, 1);
                //     lastPendingCommand->commandContent[0] = volume;
                //     break;
                case SET_SERVICE:
                    {
                        Serial.print("[BLUETOOTH] Changing service to ");
                        while (btMonitor->available() < 4);
                        uint32_t serviceId = btMonitor->read() << 24;
                        serviceId |= btMonitor->read() << 16;
                        serviceId |= btMonitor->read() << 8;
                        serviceId |= btMonitor->read();
                        receiver->startServiceWithID(serviceId);
                        addCurrentActiveServiceCommand();
                        break;
                    }
                case STOP_SERVICE:
                    Serial.println("[BLUETOOTH] Disabling the active service");
                    receiver->stopDigitalService();
                    addCurrentActiveServiceCommand();
                    break;
                case FULL_SCAN:
                    if (receiver->serviceActive) {
                        Serial.println("[BLUETOOTH] Stopping the digital service");
                        receiver->stopDigitalService();
                        addCurrentActiveServiceCommand();
                    }
                    receiver->scanAll();
                    addServiceInformationCommand();
                    break;
                case INCREASE_VOLUME:
                    muted = false;
                    if (volume != 63) {
                        volume++;
                    }
                    // Serial.print("[DEBUG] Volume doen stijgen, nieuw volume: ");
                    // Serial.println(volume);
                    receiver->setVolume(volume);
                    addCurrentVolumeCommand();
                    break;
                case DECREASE_VOLUME:
                    muted = false;
                    if (volume != 0) {
                        volume--;
                    }
                    // Serial.print("[DEBUG] Volume laten dalen, nieuw volume: ");
                    // Serial.println(volume);
                    receiver->setVolume(volume);
                    addCurrentVolumeCommand();
                    break;
                case SET_VOLUME:
                    {
                        while (btMonitor->available() == 0);
                        uint8_t newVolume = btMonitor->read();
                        if (newVolume > 63) {
                            newVolume = 63;
                        } else if (newVolume < 0) {
                            newVolume = 0;
                        }
                        volume = newVolume;
                        receiver->setVolume(volume);
                        addCurrentVolumeCommand();
                        break;
                    }
                case MUTE_AUDIO:
                    receiver->mute();
                    muted = true;
                    addCurrentVolumeCommand();
                    break;
                case UNMUTE_AUDIO:
                    receiver->unmute();
                    muted = false;
                    addCurrentVolumeCommand();
                    break;
                default:
                    Serial.print("[BLUETOOTH] Instruction nr ");
                    Serial.print(instruction);
                    Serial.println(" not yet implemented.");
                    delay(50);
                    // log("[BLUETOOTH] Instruction nr ");
                    // log(instruction);
                    // log(" not yet implemented");
                    break;
                }
        }
        // btMonitor->flush();
        while (hasPendingCommands && activeBtConnection) {
            PendingCommand cmd = nextCommand();
            send(cmd);
            free(cmd.commandContent);
        }
    }
}

PendingCommand* Radio::addCommand(uint8_t cmdHeader, uint16_t cmdLength) {
    if (currentPendingCommand == nullptr) {
        currentPendingCommand = (PendingCommand*) malloc(sizeof(PendingCommand));
        lastPendingCommand = currentPendingCommand;
    } else {
        lastPendingCommand->next = (PendingCommand*) malloc(sizeof(PendingCommand));
        lastPendingCommand = lastPendingCommand->next;
    }
    lastPendingCommand->commandLength = cmdLength;
    lastPendingCommand->commandHeader = cmdHeader;
    lastPendingCommand->commandContent = (uint8_t*) malloc(lastPendingCommand->commandLength*sizeof(uint8_t));
    lastPendingCommand->next = nullptr;
    hasPendingCommands = true;
    return lastPendingCommand;
}

PendingCommand Radio::nextCommand() {
    PendingCommand commandToSend = *currentPendingCommand;
    PendingCommand* old = currentPendingCommand;
    // commandToSend.commandContent = old->commandContent; // zou automatisch moeten gebeuren door de aanmaak van een nieuwe struct
    currentPendingCommand = currentPendingCommand->next;
    free(old);
    hasPendingCommands = currentPendingCommand != nullptr;
    return commandToSend;
}

void Radio::send(PendingCommand cmd) {
    if (activeBtConnection && btMonitor) {
        btMonitor->write(cmd.commandHeader);
        btMonitor->write((uint8_t) ((cmd.commandLength >> 8) & 0xFF));
        btMonitor->write((uint8_t) (cmd.commandLength & 0xFF));
        btMonitor->write(cmd.commandContent, cmd.commandLength);
        // Serial.print("[BLUETOOTH] Sent command code ");
        // Serial.print(cmd.commandHeader);
        // Serial.print(" with length ");
        // Serial.print(cmd.commandLength);
        // Serial.println(", and as content");
        // for (uint16_t i = 0; i < cmd.commandLength; i++) {
        //     Serial.print(cmd.commandContent[i]);
        // }
        // Serial.println();
    }
}
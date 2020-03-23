#include <BluetoothSerial.h>
#include <Radio.cpp>

class RCHandler {
    private:
        BluetoothSerial* btMonitor = nullptr;
        Radio* radio = nullptr;
        void send(uint8_t *, int); // reeks van bytes sturen
        void send(uint8_t); // 1 byte sturen
    public:
        //CONNECTIESTATUS:
        bool connected = false;
        //CONSTANTS:
        const static uint8_t SET_VOLUME = 1;
        const static uint8_t SET_CHANNEL = 2;
        //const static uint8_t ESP32_VOLUME = 3;
        //const static uint8_t ESP32_CHANNEL_LIST = 4;
        //const static uint8_t ESP32_SONG_NAME = 5;
        //const static uint8_t ESP32_SONG_ARTIST = 6;
        //const static uint8_t ESP32_SONG_ARTWORK = 7;
        //const static uint8_t ESP32_CHANNEL = 8;
        const static uint8_t REQUEST_CHANNEL_LIST = 64;
        const static uint8_t REQUEST_SONG_NAME = 65;
        const static uint8_t REQUEST_SONG_ARTIST = 66;
        const static uint8_t REQUEST_SONG_ARTWORK = 67;
        const static uint8_t REQUEST_CHANNEL = 68;
        const static uint8_t REQUEST_VOLUME = 69;

        const static uint8_t LOG = 125;
        const static uint8_t CONNECT = 126;
        const static uint8_t DISCONNECT = 127;
        //FUNCTIONS:
        /*
         * subscribeTo() : doorgeven van de Bluetooth Monitor
         * setRadio() : Radio die huidige instellingen bevat
         * update() : reageren op in- en uitgaand verkeer
         * log() : log via bluetooth doorgeven indien verbonden
        */
        void subscribeTo(BluetoothSerial* monitor) {
            btMonitor = monitor;
        }
        void setRadio(Radio* radio) {
            this->radio = radio;
        }
        void log(const char*);
        void log(uint8_t);
        void update();

        /*
         * Synchrone functies: worden uitgevoerd wanneer er
         * bepaalde aanvragen gebeuren over de monitor
         * en wordt gecontrolleerd bij elke update
        */


        /*
         * Asynchrone functies: worden uitgevoerd vanaf
         * ze worden aangesproken
        */
        void send(const char *);
};

void RCHandler::send(const char* text) {
    if (btMonitor) { // geen nullptr & controlleren op connectie
        btMonitor->println(text);
    }
}

void RCHandler::send(uint8_t* bytes, int len) {
    if (btMonitor) {
        for (uint8_t i = 0; i < len; i++) {
            btMonitor->write(bytes[i]);
        }
    }
}

void RCHandler::send(uint8_t byte) {
    if (btMonitor) {
        btMonitor->write(byte);
    }
}

void RCHandler::log(const char* logText) {
    if (connected && btMonitor) {
        int totalLength = strlen(logText);
        for (uint8_t i = 0; i < totalLength / 255; i++) {
            send(LOG);
            send(255);
            for (uint8_t j = 0; j < 255; j++) {
                send(logText[255*i + j]);
            }
        }
        send(LOG);
        send(totalLength % 255); // overige lengte van debug
        uint8_t startIndex = totalLength - totalLength % 255;
        for (uint8_t i = 0; i < totalLength % 255; i++) {
            send(logText[startIndex + i]);
        }
    }
}

void RCHandler::log(uint8_t nr) {
    uint8_t size = 1;
    if (nr > 99) {
        size = 3;
    } else if (nr > 9) {
        size = 2;
    }
    send(LOG);
    send(size);
    if (nr > 99) {
        send((char)(nr / 100 + (uint8_t) '0'));
    }
    if (nr > 9) {
        send((char)((nr / 10 ) - (10 * (nr / 100)) + (uint8_t) '0'));
    }
    send((char) (nr % 10 + (uint8_t) '0'));
}

void RCHandler::update() {
    if (btMonitor->available()) {
        while (btMonitor->available()) {
            uint8_t instruction = btMonitor->read();
            switch (instruction) {
                case CONNECT:
                    connected = true;
                    Serial.println("[BLUETOOTH] A device has been connected");
                    delay(50);
                    log("[BLUETOOTH] A device has been connected\n");
                    break;
                case DISCONNECT:
                    connected = false;
                    Serial.println("[BLUETOOTH] Device disconnected");
                    break;
                case REQUEST_VOLUME:
                    Serial.println("[BLUETOOTH] Sending current volume");
                    send(radio->volume);
                    delay(50);
                    log("[BLUETOOTH] Sending current volume\n");
                    break;
                case SET_VOLUME:
                    Serial.print("[BLUETOOTH] Setting volume to ");
                    radio->volume = btMonitor->read();
                    Serial.print(radio->volume);
                    Serial.println("%.");
                    delay(50);
                    log("[BLUETOOTH] Setting volume to ");
                    log(radio->volume);
                    log("%.\n");
                    break;
                default:
                    Serial.print("[BLUETOOTH] Instruction nr ");
                    Serial.print(instruction);
                    Serial.println(" not yet implemented.");
                    delay(50);
                    log("[BLUETOOTH] Instruction nr ");
                    log(instruction);
                    log(" not yet implemented");
                    break;
            }
        }
        btMonitor->flush();
    }
}
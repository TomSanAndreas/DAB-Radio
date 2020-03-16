#include <BluetoothSerial.h>

class RCHandler {
    private:
        BluetoothSerial* btMonitor = nullptr;
    public:
        //CONSTANTS:
        const uint8_t SET_VOLUME = 1;
        const uint8_t SET_CHANNEL = 2;
        const uint8_t REQUEST_CHANNEL_LIST = 128;
        const uint8_t REQUEST_SONG_NAME = 129;
        const uint8_t REQUEST_SONG_ARTIST = 130;
        const uint8_t REQUEST_SONG_ARTWORK = 131;
        const uint8_t REQUEST_CHANNEL = 132;
        const uint8_t REQUEST_VOLUME = 133;
        //FUNCTIONS:
        /*
         * subscribeTo() : doorgeven van de Bluetooth Monitor
         * update() : reageren op in- en uitgaand verkeer
        */
        void subscribeTo(BluetoothSerial*);
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

void RCHandler::subscribeTo(BluetoothSerial* monitor) {
    btMonitor = monitor;
}

void RCHandler::send(const char* text) {
    if (btMonitor) { // geen nullptr & controlleren op connectie
        (*btMonitor).println(text);
    }
}

void RCHandler::update() {}
#include <BluetoothSerial.h>
#include <Radio.h>

class RCHandler {
    private:
        BluetoothSerial* btMonitor;
        Radio* radio;
        void send(uint8_t *, int);
    public:
        //CONSTANTS:
        const static uint8_t SET_VOLUME = 1;
        const static uint8_t SET_CHANNEL = 2;
        const static uint8_t ESP32_VOLUME = 3;
        const static uint8_t ESP32_CHANNEL_LIST = 4;
        const static uint8_t ESP32_SONG_NAME = 5;
        const static uint8_t ESP32_SONG_ARTIST = 6;
        const static uint8_t ESP32_SONG_ARTWORK = 7;
        const static uint8_t ESP32_CHANNEL = 8;
        const static uint8_t REQUEST_CHANNEL_LIST = 64;
        const static uint8_t REQUEST_SONG_NAME = 65;
        const static uint8_t REQUEST_SONG_ARTIST = 66;
        const static uint8_t REQUEST_SONG_ARTWORK = 67;
        const static uint8_t REQUEST_CHANNEL = 68;
        const static uint8_t REQUEST_VOLUME = 69;
        //FUNCTIONS:
        /*
         * subscribeTo() : doorgeven van de Bluetooth Monitor
         * setRadio() : Radio die huidige instellingen bevat
         * update() : reageren op in- en uitgaand verkeer
        */
        void subscribeTo(BluetoothSerial* monitor);
        void setRadio(Radio* radio);
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

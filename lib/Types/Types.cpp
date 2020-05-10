#include <Arduino.h>

typedef struct Component {
    uint32_t component_id; // gebruikt om digital service te starten
} Component;

typedef struct Service {
    uint32_t service_id; // ID van de service, gebruikt om digital service te starten
    char service_label[17]; // naam van het kanaal
    uint8_t nComponenten; // aantal componenten die het kanaal bevat
    Component* componenten; // pointer naar het eerste component
} Service;

typedef struct PendingCommand {
    uint8_t commandHeader;
    uint8_t* commandContent; // command content weer vrijgeven in de RCHandler!
    uint16_t commandLength; // zonder commandoHeader meegerekend
    PendingCommand* next = nullptr;
} PendingCommand;

#pragma once
#include <ArduinoJson.h>
#include "config.h"
#include "storage.h"
#include "sensors.h"
#include "rtc_manager.h"
#include "relays.h"
namespace paulett {
struct Event { uint32_t time; char code[24],description[100]; };
class App {
 Storage store; uint32_t lastElapsed=0,dayLogged=0; bool recovered=false,sensorReported=false,wasSensor=false,wasClock=false,overheated=false;
 uint32_t manualUntil[4]={0};
 Event events[48]{}; uint8_t eventHead=0,eventCount=0;
 bool persist(); void critical(const char*); void motorStop(bool complete); bool motorStart(bool manual,String& error);
 void environment(uint32_t); void recover();
 public:
 Config config; Cycle cycle; Sensors sensors; Clock clock; Relays relays;
 bool storageOK=false,fsOK=false; String lcdEvent; uint32_t lcdEventUntil=0;
 void begin(); void tick(); void event(const char*,const char*);
 bool command(const JsonDocument&,String& error);
 void status(JsonDocument&); void settings(JsonDocument&);
 uint32_t logicalElapsed() const;
 const char* stateName() const;
};
extern App app;
extern SemaphoreHandle_t appMutex;
}

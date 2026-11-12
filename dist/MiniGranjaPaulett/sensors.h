#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "config.h"
namespace paulett {
class Sensors {
 DHT dht{4,DHT22}; uint32_t polled=0; bool first=true;
 public:
 float temperature=NAN,humidity=NAN; uint32_t lastValid=0; bool lastReadOK=false,hasValue=false;
 void begin() { dht.begin(); }
 void tick(uint32_t ms,bool relaysSettled=true);
 bool reliable(uint32_t ms) const { return hasValue&&lastReadOK&&ms-lastValid<=SENSOR_STALE_MS; }
};
}

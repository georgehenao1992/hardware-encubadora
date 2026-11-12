#pragma once
#include <RTClib.h>
namespace paulett {
class Clock {
 RTC_DS3231 rtc; uint32_t polled=0,previous=0,previousMs=0,advancedMs=0; bool first=true;
 public:
 bool present=false,valid=false,jumped=false; uint32_t now=0;
 void begin(); void tick(uint32_t); bool adjust(uint32_t epoch); String diagnostic();
};
}

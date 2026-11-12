#include "rtc_manager.h"
#include <Wire.h>
namespace paulett {
static bool plausibleDate(uint32_t t) { return t>=1735689600UL && t<4102444800UL; }
void Clock::begin() { present=rtc.begin(); tick(millis()); }
void Clock::tick(uint32_t ms) {
 if(!first && ms-polled<1000) return;
 first=false; polled=ms;
 Wire.beginTransmission(0x68);
 present=Wire.endTransmission()==0;
 if(!present) { valid=false; return; }
 uint32_t candidate=rtc.now().unixtime();
 valid=!rtc.lostPower()&&plausibleDate(candidate);
 if(valid) {
   if(previous && (candidate<previous || abs(int64_t(candidate)-previous-int64_t((ms-previousMs)/1000))>5)) jumped=true;
   if(candidate!=previous) advancedMs=ms;
   else if(ms-advancedMs>5000) jumped=true;
   previous=candidate; previousMs=ms; now=candidate;
 }
}
bool Clock::adjust(uint32_t epoch) {
 if(!present||!plausibleDate(epoch)) return false;
 rtc.adjust(DateTime(epoch)); previous=0; jumped=false; first=true; tick(millis());
 return valid&&now==epoch;
}
String Clock::diagnostic() {
 String result="RTC presente="+String(present)+" valido="+String(valid)+" salto="+String(jumped)+" epoch="+String(now);
 Wire.beginTransmission(0x68); Wire.write(uint8_t(0));
 uint8_t code=Wire.endTransmission(false);
 result+=" I2C="+String(code);
 if(code==0) {
  uint8_t count=Wire.requestFrom(uint8_t(0x68),uint8_t(16));
  result+=" bytes="+String(count)+" registros=";
  while(Wire.available()) { char hex[4]; snprintf(hex,sizeof(hex),"%02X ",Wire.read()); result+=hex; }
 }
 return result;
}

}

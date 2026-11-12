#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include "config.h"
namespace paulett {
class Relays {
 bool armed=false;
 bool low[4]={false,false,false,false}; uint32_t changed[4]={0};
 esp_timer_handle_t motorTimer=nullptr;
 static void timeout(void*);
 public:
 bool on[4]={false,false,false,false}; Mode modes[4]={Mode::DESHABILITADO,Mode::DESHABILITADO,Mode::DESHABILITADO,Mode::DESHABILITADO};
 volatile bool expired=false;
 bool begin(const Config&); void allOff(); bool set(uint8_t,bool,uint32_t,bool force=false);
 bool startMotor(uint32_t seconds,uint32_t ms); void stopMotor();
 // Evitar iniciar una trama DHT durante el transitorio de conmutacion.
 bool settled(uint32_t ms) const {
  for(uint32_t at:changed) if(uint32_t(ms-at)<250) return false;
  return true;
 }
};
}

#include "relays.h"
namespace paulett {
void Relays::timeout(void* ctx) {
 auto* r=static_cast<Relays*>(ctx);
#ifndef PAULETT_SIMULATION
 digitalWrite(PINS[0],r->low[0]?HIGH:LOW);
#endif
 r->expired=true;
}
bool Relays::begin(const Config& c) {
 armed=c.hardwareConfirmed;
 for(int i=0;i<4;i++) {
   low[i]=c.activeLow[i];
#ifndef PAULETT_SIMULATION
   // Cargar el latch ANTES de habilitar el GPIO como salida.
   if(armed) { digitalWrite(PINS[i],low[i]?HIGH:LOW); pinMode(PINS[i],OUTPUT); } else { pinMode(PINS[i],INPUT); }
#endif
   on[i]=false; changed[i]=millis();
 }
 if(motorTimer) return true;
 esp_timer_create_args_t args{}; args.callback=timeout; args.arg=this; args.name="motor_limit";
 return esp_timer_create(&args,&motorTimer)==ESP_OK;
}
bool Relays::set(uint8_t i,bool value,uint32_t ms,bool force) {
 if(i>3||on[i]==value) return false;
 if(!force && ms-changed[i]<SWITCH_MS) return false;
#ifndef PAULETT_SIMULATION
 if(armed) digitalWrite(PINS[i],value?(low[i]?LOW:HIGH):(low[i]?HIGH:LOW));
#endif
 on[i]=value; changed[i]=ms; return true;
}
void Relays::allOff() { stopMotor(); for(int i=1;i<4;i++) set(i,false,millis(),true); }
bool Relays::startMotor(uint32_t sec,uint32_t ms) {
 if(!motorTimer||sec<1||sec>ABS_MOTOR_SECONDS||on[0]) return false;
 expired=false;
 if(esp_timer_start_once(motorTimer,uint64_t(sec)*1000000)!=ESP_OK) return false;
 set(0,true,ms,true); return true;
}
void Relays::stopMotor() { if(motorTimer) esp_timer_stop(motorTimer); set(0,false,millis(),true); expired=false; }
}

#include "sensors.h"
namespace paulett {
void Sensors::tick(uint32_t ms,bool relaysSettled) {
 if(first) { first=false; polled=ms; return; }
 if(ms-polled<2500) return;
 // Aplazar solo la adquisicion, sin bloquear la tarea ni renovar lastValid.
 // Una lectura fallida sigue invalidando el sensor inmediatamente.
 if(!relaysSettled) return;
 polled=ms;
 float h=dht.readHumidity(),t=dht.readTemperature();
 lastReadOK=plausible(t,h);
 if(lastReadOK) { temperature=t; humidity=h; lastValid=ms; hasValue=true; }
}
}

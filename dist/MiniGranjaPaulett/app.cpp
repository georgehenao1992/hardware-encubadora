#include "app.h"
#include <esp_system.h>
#include <Wire.h>
namespace paulett {
App app; SemaphoreHandle_t appMutex;
static const char* names[]={"SIN_CICLO","INCUBANDO","PAUSADO","NACIMIENTO","FINALIZADO","ERROR_CRITICO"};
static const char* relayNames[]={"MOTOR","CALEFACCION","VENTILADOR","HUMIDIFICADOR"};
static const char* modeNames[]={"AUTOMATICO","MANUAL_TEMPORAL","DESHABILITADO","BLOQUEADO_POR_SEGURIDAD"};
const char* App::stateName() const { return names[static_cast<int>(cycle.state)]; }
void App::event(const char* code,const char* description) {
 Event& e=events[eventHead]; e.time=clock.valid?clock.now:0;
 strlcpy(e.code,code,sizeof(e.code)); strlcpy(e.description,description,sizeof(e.description));
 eventHead=(eventHead+1)%48; if(eventCount<48) eventCount++;
 lcdEvent=code; lcdEventUntil=millis()+4500;
 Serial.printf("[%lu] %s: %s\n",static_cast<unsigned long>(e.time),code,description);
}
void App::critical(const char* reason) {
 storageOK=false; relays.allOff(); cycle.state=State::ERROR_CRITICO; event("ERROR_CRITICO",reason);
}
bool App::persist() {
 if(!storageOK||!store.saveCycle(cycle)) { critical("No se pudo persistir el ciclo; salidas bloqueadas"); return false; }
 return true;
}
void App::begin() {
 bool fresh=false; storageOK=store.begin()&&store.load(config,cycle,fresh);
 if(!storageOK) { config=Config{}; cycle=Cycle{}; cycle.state=State::ERROR_CRITICO; }
 if(fresh||!storageOK) {
  strlcpy(config.apPassword,INITIAL_PASSWORD,sizeof(config.apPassword));
  strlcpy(config.adminPassword,INITIAL_PASSWORD,sizeof(config.adminPassword));
  Serial.printf("\nCREDENCIALES PRIVADAS - conservar\nWiFi MINI_GRANJA_PAULETT: %s\nAdministrador admin: %s\n",config.apPassword,config.adminPassword);
 }
 if(fresh&&storageOK) storageOK=store.saveConfig(config)&&store.saveCycle(cycle);
 if(storageOK) storageOK=store.applyCredentialPreset(config);
 if(storageOK) storageOK=store.applyActiveHighPreset(config);
 if(!relays.begin(config)) critical("No se pudo crear el temporizador del motor");
 Wire.begin(21,22); Wire.setTimeOut(30);
 String found;
 for(uint8_t a=1;a<127;a++) { Wire.beginTransmission(a); if(Wire.endTransmission()==0) { char buf[8]; snprintf(buf,sizeof(buf),"0x%02X ",a); found+=buf; } }
 Serial.println("I2C detectado: "+found);
 clock.begin(); sensors.begin();
 char reset[90]; snprintf(reset,sizeof(reset),"Motivo ESP32=%d; arranque con salidas apagadas",int(esp_reset_reason()));
 event("REINICIO",reset); event("I2C",found.c_str());
 if(!clock.valid) event("ERROR RTC","RTC ausente o sin hora valida; ajustar desde el panel");
 if(!storageOK) critical("NVS invalida o incompatible: revisar por USB, no se sobrescribe");
 recover();
}
uint32_t App::logicalElapsed() const { return clock.valid&&!clock.jumped?elapsed(cycle,clock.now):lastElapsed; }
void App::recover() {
 if(recovered||!storageOK) return;
 recovered=true;
 if(cycle.motorPending) { cycle.motorPending=false; event("MOTOR INCIERTO","Intento previo bloqueado; no se repite tras reinicio"); if(!persist()) return; }
 if(existing(cycle.state)) {
  if(!clock.valid||clock.now<cycle.resumedAt) {
   lastElapsed=cycle.elapsedSaved; cycle.state=State::PAUSADO; cycle.pauseStarted=0;
   event("REVISAR RELOJ","Tiempo congelado en ultimo estado persistido; revisar duracion antes de reanudar"); persist();
  } else {
   lastElapsed=elapsed(cycle,clock.now);
   if(running(cycle.state)) {
    cycle.state=stageAt(lastElapsed,config);
    if(cycle.state==State::FINALIZADO) { cycle.elapsedSaved=endSeconds(config); event("CICLO TERMINADO","Finalizo durante apagado"); }
    cycle.nextTurnElapsed=futureSlot(cycle.nextTurnElapsed,lastElapsed,config.intervalSeconds);
    persist();
   }
   event("RECUPERACION","Programacion reconstruida; movimientos vencidos omitidos");
  }
 }
}
void App::motorStop(bool complete) {
 if(!relays.on[0]&&!cycle.motorPending) return;
 relays.stopMotor();
 if(complete) cycle.lastConfirmed=clock.valid?clock.now:cycle.lastAttempt;
 cycle.motorPending=false;
 persist(); event("MOTOR APAGADO",complete?"Movimiento completado":"Movimiento interrumpido; no se repetira");
}
bool App::motorStart(bool manual,String& error) {
 uint32_t e=logicalElapsed();
 if(!storageOK||!config.hardwareConfirmed||!clock.valid||clock.jumped||!sensors.reliable(millis())||sensors.temperature>=config.overtemp
    ||cycle.state!=State::INCUBANDO||e>=HATCH||relays.on[0]||cycle.motorPending) { error="Motor bloqueado por seguridad o etapa"; return false; }
 // Registrar y consumir la ranura ANTES de alimentar el motor; NVS es la barrera de seguridad.
 cycle.lastAttempt=clock.now; cycle.motorPending=true;
 cycle.nextTurnElapsed=futureSlot(cycle.nextTurnElapsed,e,config.intervalSeconds);
 if(manual) cycle.nextTurnElapsed=e+config.intervalSeconds;
 if(!persist()) { error="Error NVS"; return false; }
 if(!relays.startMotor(config.motorSeconds,millis())) { motorStop(false); error="Fallo temporizador"; return false; }
 relays.modes[0]=manual?Mode::MANUAL_TEMPORAL:Mode::AUTOMATICO;
 event("MOTOR ACTIVADO",manual?"Prueba temporizada; siguiente movimiento reprogramado":"Movimiento automatico"); return true;
}
void App::environment(uint32_t ms) {
 bool safe=storageOK&&config.hardwareConfirmed&&cycle.state!=State::ERROR_CRITICO;
 bool sensor=sensors.reliable(ms);
 bool allowed=existing(cycle.state)||config.idlePolicy==IdlePolicy::REGULATE;
 uint32_t e=logicalElapsed(); const Stage& s=e>=HATCH?config.birth:config.incubation;
 Demand d{false,false,false};
 if(safe&&sensor&&allowed) d=regulate(s,sensors.temperature,sensors.humidity,{relays.on[1],relays.on[2],relays.on[3]});
 bool hot=sensor&&sensors.temperature>=config.overtemp;
 if(hot!=overheated) { overheated=hot; if(hot) event("SOBRETEMP","Umbral de emergencia: calefaccion y motor bloqueados"); else event("TEMP NORMAL","Sobretemperatura resuelta"); }
 if(safe&&(!sensor||hot)) { d.heat=false; d.humid=false; d.fan=hot||config.emergencyFan; }
 bool desired[4]={false,d.heat,d.fan,d.humid};
 for(int i=1;i<4;i++) {
  bool manual=manualUntil[i]&&int32_t(manualUntil[i]-ms)>0;
  if(!manual) manualUntil[i]=0;
  if(manual&&safe&&sensor&&!hot) {
   // Una prueba tampoco ignora los limites de su actuador.
   desired[i]=i==1?sensors.temperature<s.heatOff:(i==3?sensors.humidity<s.humidOff:true);
  }
  relays.modes[i]=!safe||!sensor||hot?Mode::BLOQUEADO_POR_SEGURIDAD:(manual?Mode::MANUAL_TEMPORAL:(allowed?Mode::AUTOMATICO:Mode::DESHABILITADO));
 }
 if(desired[2]) desired[1]=false;
 // Apagar el antagonista inmediatamente antes de permitir el encendido.
 if(desired[2]&&relays.on[1]) { relays.set(1,false,ms,true); event("CALEFACCION OFF","Prioridad ventilacion"); }
 if(desired[1]&&relays.on[2]) desired[1]=false;
 for(int i=1;i<4;i++) {
  bool force=!desired[i]&&(!safe||!sensor||hot||!allowed||manualUntil[i]==0);
  if(relays.set(i,desired[i],ms,force)) { String msg=String(relayNames[i])+(desired[i]?" ON":" OFF"); event(msg.c_str(),"Cambio de salida ambiental"); }
 }
}
void App::tick() {
 uint32_t ms=millis();
 if(relays.expired) motorStop(true);
 sensors.tick(ms,relays.settled(ms)); clock.tick(ms);
 bool good=sensors.reliable(ms);
 if(good!=wasSensor||(!sensorReported&&ms>=3000)) { sensorReported=true; wasSensor=good; event(good?"SENSOR OK":"ERROR SENSOR",good?"Lecturas confiables":"Calefaccion y humedad apagadas; revisar DHT22"); }
 bool rtcGood=clock.valid&&!clock.jumped;
 if(rtcGood!=wasClock) { wasClock=rtcGood; event(rtcGood?"RELOJ OK":"ERROR RTC",rtcGood?"Referencia temporal valida":"Reloj ausente, invalido o salto temporal"); }
 if(!rtcGood&&running(cycle.state)) {
  cycle.elapsedSaved=lastElapsed; cycle.state=State::PAUSADO; cycle.pauseStarted=0;
  motorStop(false); persist(); event("PAUSA SEGURA","Perdida de reloj; requiere ajuste y reanudacion manual");
 }
 if(rtcGood&&running(cycle.state)&&storageOK) {
  uint32_t e=elapsed(cycle,clock.now); lastElapsed=e;
  State target=stageAt(e,config);
  if(target!=cycle.state) {
   cycle.state=target; motorStop(false);
   if(target==State::FINALIZADO) cycle.elapsedSaved=endSeconds(config);
   persist(); event(target==State::FINALIZADO?"CICLO TERMINADO":"NACIMIENTO","Volteo deshabilitado");
  }
  uint32_t day=min(e/DAY+1,uint32_t(21));
  if(day!=dayLogged) { dayLogged=day; char msg[40]; snprintf(msg,sizeof(msg),"Dia %lu del ciclo",static_cast<unsigned long>(day)); event("CAMBIO DIA",msg); }
  if(cycle.state==State::INCUBANDO&&e>=cycle.nextTurnElapsed&&!relays.on[0]) {
   // Una demora mayor a 5 s consume la ranura, nunca acumula movimientos.
   if(config.automaticTurn&&e-cycle.nextTurnElapsed<=5) { String err; if(!motorStart(false,err)) { cycle.nextTurnElapsed=futureSlot(cycle.nextTurnElapsed,e,config.intervalSeconds); persist(); event("VOLTEO OMITIDO",err.c_str()); } }
   else { cycle.nextTurnElapsed=futureSlot(cycle.nextTurnElapsed,e,config.intervalSeconds); persist(); }
  }
 }
 if(relays.on[0]&&(!rtcGood||!good||sensors.temperature>=config.overtemp||cycle.state!=State::INCUBANDO||logicalElapsed()>=HATCH)) motorStop(false);
 if(!relays.on[0]) relays.modes[0]=!config.hardwareConfirmed||!rtcGood||!good?Mode::BLOQUEADO_POR_SEGURIDAD:(cycle.state==State::INCUBANDO&&config.automaticTurn?Mode::AUTOMATICO:Mode::DESHABILITADO);
 environment(ms);
}
static void stageJson(JsonObject o,const Stage& s) {
 o["heatOn"]=s.heatOn; o["heatOff"]=s.heatOff; o["fanOn"]=s.fanOn; o["fanOff"]=s.fanOff; o["humidOn"]=s.humidOn; o["humidOff"]=s.humidOff;
}
void App::settings(JsonDocument& doc) {
 stageJson(doc.createNestedObject("incubation"),config.incubation); stageJson(doc.createNestedObject("birth"),config.birth);
 doc["motorSeconds"]=config.motorSeconds; doc["motorMaxSeconds"]=config.motorMaxSeconds; doc["intervalSeconds"]=config.intervalSeconds;
 doc["firstMovement"]=config.firstMovement; doc["automaticTurn"]=config.automaticTurn; doc["finishAt18"]=config.finishAt18;
 doc["overtemp"]=config.overtemp; doc["emergencyFan"]=config.emergencyFan; doc["idlePolicy"]=int(config.idlePolicy);
 doc["hardwareConfirmed"]=config.hardwareConfirmed; doc["lcdAddress"]=config.lcdAddress;
 auto a=doc.createNestedArray("activeLow"); for(bool low:config.activeLow) a.add(low);
}
void App::status(JsonDocument& doc) {
 uint32_t ms=millis(),e=min(logicalElapsed(),endSeconds(config));
 doc["state"]=stateName(); doc["elapsed"]=e; doc["day"]=cycle.startedAt?min(e/DAY+1,uint32_t(21)):0;
 doc["remaining"]=cycle.state==State::FINALIZADO?0:endSeconds(config)-e;
 doc["temperature"]=sensors.temperature; doc["humidity"]=sensors.humidity; doc["sensorOK"]=sensors.reliable(ms);
 doc["sensorAge"]=sensors.hasValue?int64_t(ms-sensors.lastValid):-1;
 doc["rtcOK"]=clock.valid&&!clock.jumped; doc["now"]=clock.valid?clock.now:0;
 doc["storageOK"]=storageOK; doc["hardwareConfirmed"]=config.hardwareConfirmed; doc["fsOK"]=fsOK;
 doc["idlePolicy"]=int(config.idlePolicy); doc["emergencyFan"]=config.emergencyFan;
 doc["lastAttempt"]=cycle.lastAttempt; doc["lastConfirmed"]=cycle.lastConfirmed;
 doc["startedAt"]=cycle.startedAt; doc["pauseTotal"]=cycle.pauseTotal;
 doc["nextTurn"]=cycle.state==State::INCUBANDO&&config.automaticTurn&&clock.valid&&!clock.jumped&&cycle.nextTurnElapsed<HATCH ? clock.now+(cycle.nextTurnElapsed>e?cycle.nextTurnElapsed-e:0):0;
#ifdef PAULETT_SIMULATION
 doc["simulation"]=true;
#else
 doc["simulation"]=false;
#endif
 auto outputs=doc.createNestedArray("relays");
 for(int i=0;i<4;i++) { auto o=outputs.createNestedObject(); o["name"]=relayNames[i]; o["on"]=relays.on[i]; o["activeLow"]=config.activeLow[i];
#ifndef PAULETT_SIMULATION
 if(config.hardwareConfirmed) o["gpioLevel"]=digitalRead(PINS[i])==HIGH?"HIGH":"LOW";
#endif
 o["mode"]=modeNames[int(relays.modes[i])]; }
 auto alarms=doc.createNestedArray("alarms");
 if(!sensors.reliable(ms)) alarms.add("ERROR SENSOR: calefaccion y humidificacion bloqueadas");
 if(!clock.valid||clock.jumped) alarms.add("ERROR RTC: ajuste el reloj y revise el ciclo");
 if(!config.hardwareConfirmed) alarms.add("Instalacion electrica pendiente de confirmar; salidas bloqueadas");
 if(!storageOK) alarms.add("ERROR NVS: todas las salidas bloqueadas");
 if(overheated) alarms.add("SOBRETEMPERATURA");
 if(sensors.reliable(ms)&&existing(cycle.state)) { const Stage& s=e>=HATCH?config.birth:config.incubation;
  if(sensors.temperature<s.heatOn-1||sensors.temperature>s.fanOn) alarms.add("Temperatura fuera del rango objetivo");
  if(sensors.humidity<s.humidOn-5||sensors.humidity>s.humidOff+5) alarms.add("Humedad fuera del rango objetivo");
 }
 auto history=doc.createNestedArray("events");
 for(int i=0;i<eventCount;i++) { const auto& ev=events[(eventHead+47-i)%48]; auto o=history.createNestedObject(); o["time"]=ev.time; o["code"]=ev.code; o["description"]=ev.description; }
}
static bool uintField(JsonVariantConst v,uint32_t lo,uint32_t hi) { return v.is<uint32_t>()&&v.as<uint32_t>()>=lo&&v.as<uint32_t>()<=hi; }
static bool readStage(JsonVariantConst v,Stage& s) {
 const char* keys[]={"heatOn","heatOff","fanOn","fanOff","humidOn","humidOff"};
 float* fields[]={&s.heatOn,&s.heatOff,&s.fanOn,&s.fanOff,&s.humidOn,&s.humidOff};
 for(int i=0;i<6;i++) { if(!v[keys[i]].is<float>()) return false; *fields[i]=v[keys[i]].as<float>(); } return stageValid(s);
}
bool App::command(const JsonDocument& d,String& error) {
 auto fail=[&](const char* msg){error=msg;return false;};
 if(!d["confirm"].is<bool>()||!d["confirm"].as<bool>()) return fail("Confirmacion requerida");
 String action=d["action"]|"";
 if(action=="clock") {
  if(running(cycle.state)||relays.on[0]) return fail("Pause el ciclo antes de ajustar el reloj");
  if(!uintField(d["epoch"],1735689600UL,4102444799UL)||!clock.adjust(d["epoch"])) return fail("Fecha invalida o RTC ausente");
  if(cycle.state==State::PAUSADO) { cycle.pauseStarted=clock.now; if(!persist()) return fail("Error NVS"); }
  event("AJUSTE RELOJ","Fecha y hora local actualizadas"); return true;
 }
 if(!storageOK) return fail("NVS bloqueada: mantenimiento por USB requerido");
 if(action=="start"||action=="new") {
  if(existing(cycle.state)) return fail("Finalice el ciclo anterior");
  if(!clock.valid||clock.jumped||!sensors.reliable(millis())||!config.hardwareConfirmed) return fail("Confirme hardware y revise sensor/reloj");
  if(!uintField(d["start"],1735689600UL,clock.now)) return fail("Inicio debe ser una fecha valida no futura");
  uint32_t start=d["start"],age=clock.now-start;
  if(age>=endSeconds(config)) return fail("La fecha inicial ya supera la duracion del ciclo");
  if(config.firstMovement&&(config.firstMovement<start||config.firstMovement-start>=HATCH)) return fail("Primer movimiento debe caer dentro de las primeras 432 horas");
  cycle=Cycle{}; cycle.startedAt=start; cycle.resumedAt=start; cycle.state=stageAt(age,config);
  cycle.nextTurnElapsed=config.firstMovement?config.firstMovement-start:config.intervalSeconds;
  cycle.nextTurnElapsed=futureSlot(cycle.nextTurnElapsed,age,config.intervalSeconds);
  lastElapsed=age; dayLogged=0; if(!persist()) return fail("Error NVS");
  event("INICIO CICLO","Ciclo de gallina iniciado"); return true;
 }
 if(action=="pause") {
  if(!running(cycle.state)) return fail("No hay un ciclo en marcha");
  cycle.elapsedSaved=logicalElapsed(); lastElapsed=cycle.elapsedSaved; cycle.pauseStarted=clock.valid?clock.now:0; cycle.state=State::PAUSADO;
  motorStop(false); if(!persist()) return fail("Error NVS");
  event("CICLO PAUSADO","Tiempo congelado; control ambiental permanece activo"); return true;
 }
 if(action=="resume") {
  if(cycle.state!=State::PAUSADO||!clock.valid||clock.jumped||!sensors.reliable(millis())||!config.hardwareConfirmed) return fail("Reanudacion bloqueada; revise estado, reloj, sensor y hardware");
  if(cycle.pauseStarted&&clock.now>=cycle.pauseStarted) cycle.pauseTotal+=clock.now-cycle.pauseStarted;
  cycle.resumedAt=clock.now; cycle.state=stageAt(cycle.elapsedSaved,config); cycle.pauseStarted=0;
  cycle.nextTurnElapsed=futureSlot(cycle.nextTurnElapsed,cycle.elapsedSaved,config.intervalSeconds);
  if(!persist()) return fail("Error NVS");
  event("CICLO REANUDADO","Se conserva el tiempo acumulado"); return true;
 }
 if(action=="finish") {
  if(!existing(cycle.state)) return fail("No hay ciclo activo");
  cycle.elapsedSaved=logicalElapsed(); cycle.state=State::FINALIZADO; motorStop(false);
  for(auto& t:manualUntil) t=0;
  if(!persist()) return fail("Error NVS");
  environment(millis()); event("CICLO TERMINADO","Finalizacion manual; politica ambiental aplicada"); return true;
 }
 if(action=="test") {
  if(!uintField(d["relay"],0,3)) return fail("Rele invalido");
  int i=d["relay"];
  if(i==0) return motorStart(true,error);
  if(!config.hardwareConfirmed||!sensors.reliable(millis())||sensors.temperature>=config.overtemp) return fail("Prueba bloqueada por seguridad");
  if(!uintField(d["seconds"],1,30)) return fail("Prueba permitida entre 1 y 30 segundos");
  if(manualUntil[i]&&int32_t(manualUntil[i]-millis())>0) return fail("Prueba ya en curso");
  manualUntil[i]=millis()+d["seconds"].as<uint32_t>()*1000;
  environment(millis()); event("PRUEBA RELE",relayNames[i]); return true;
 }
 if(action=="stopTests") { for(auto& t:manualUntil) t=0; motorStop(false); relays.allOff(); event("FIN PRUEBAS","Salidas vuelven al control automatico"); return true; }
 if(action=="config") {
  if(existing(cycle.state)||relays.on[0]) return fail("Finalice el ciclo antes de cambiar parametros");
  for(auto t:manualUntil) if(t&&int32_t(t-millis())>0) return fail("Detenga las pruebas antes de configurar");
  Config c=config;
  if(!readStage(d["incubation"],c.incubation)||!readStage(d["birth"],c.birth)) return fail("Umbrales invalidos: revise histeresis y prioridad de ventilacion");
  if(!uintField(d["motorSeconds"],1,120)||!uintField(d["motorMaxSeconds"],1,120)||!uintField(d["intervalSeconds"],600,DAY)||!uintField(d["firstMovement"],0,4102444799UL)) return fail("Temporizacion de motor invalida");
  const char* bools[]={"automaticTurn","finishAt18","emergencyFan","hardwareConfirmed"};
  for(auto key:bools) if(!d[key].is<bool>()) return fail("Tipo booleano requerido");
  if(!d["overtemp"].is<float>()||!uintField(d["idlePolicy"],0,1)||!uintField(d["lcdAddress"],0x27,0x3f)) return fail("Configuracion invalida");
  if(d["activeLow"].size()!=4) return fail("Defina las cuatro polaridades");
  for(int i=0;i<4;i++) { if(!d["activeLow"][i].is<bool>()) return fail("Polaridad invalida"); c.activeLow[i]=d["activeLow"][i]; }
  c.motorSeconds=d["motorSeconds"]; c.motorMaxSeconds=d["motorMaxSeconds"]; c.intervalSeconds=d["intervalSeconds"]; c.firstMovement=d["firstMovement"];
  c.automaticTurn=d["automaticTurn"]; c.finishAt18=d["finishAt18"]; c.emergencyFan=d["emergencyFan"]; c.hardwareConfirmed=d["hardwareConfirmed"];
  c.overtemp=d["overtemp"]; c.idlePolicy=IdlePolicy(d["idlePolicy"].as<uint8_t>()); c.lcdAddress=d["lcdAddress"];
  if(!valid(c)) return fail("Parametros fuera de limites o incompatibles");
  relays.allOff();
  if(!store.saveConfig(c)) { critical("Fallo al guardar configuracion"); return fail("Error NVS"); }
  config=c; if(!relays.begin(config)) { critical("Fallo temporizador"); return fail("Error temporizador"); }
  event("CONFIGURACION","Parametros guardados; direccion LCD aplica al reiniciar"); return true;
 }
 if(action=="credentials") {
  if(existing(cycle.state)) return fail("Finalice el ciclo antes de cambiar credenciales");
  String wifi=d["wifi"]|"",admin=d["admin"]|"";
  if(wifi.length()<8||wifi.length()>63||admin.length()<8||admin.length()>63) return fail("WiFi: 8–63 caracteres; administrador: 8–63");
  for(char ch:wifi) if(ch<33||ch>126) return fail("Use caracteres ASCII sin espacios");
  for(char ch:admin) if(ch<33||ch>126) return fail("Use caracteres ASCII sin espacios");
  Config c=config; strlcpy(c.apPassword,wifi.c_str(),sizeof(c.apPassword)); strlcpy(c.adminPassword,admin.c_str(),sizeof(c.adminPassword));
  if(!store.saveConfig(c)) { critical("No se pudieron guardar credenciales"); return fail("Error NVS"); }
  config=c; event("CREDENCIALES","Clave administrativa actualizada; WiFi aplica al reiniciar"); return true;
 }
#ifdef PAULETT_SIMULATION
 if(action=="simulate") {
  if(!existing(cycle.state)||!uintField(d["elapsed"],0,END)||!clock.valid) return fail("Inicie un ciclo y seleccione 0–504 horas");
  motorStop(false); cycle.elapsedSaved=d["elapsed"]; lastElapsed=cycle.elapsedSaved; cycle.resumedAt=clock.now; cycle.state=stageAt(cycle.elapsedSaved,config);
  cycle.nextTurnElapsed=futureSlot(0,cycle.elapsedSaved,config.intervalSeconds);
  if(!persist()) return fail("Error NVS");
  event("SIMULACION","Salidas fisicas deshabilitadas por compilacion"); return true;
 }
#endif
 return fail("Accion desconocida");
}
}

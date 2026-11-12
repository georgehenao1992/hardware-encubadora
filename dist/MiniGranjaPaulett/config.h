#pragma once
#include <stdint.h>
#include <math.h>
namespace paulett {
constexpr uint8_t PINS[4] = {25,26,27,33};
constexpr uint32_t DAY=86400, HATCH=18*DAY, END=21*DAY;
constexpr uint32_t ABS_MOTOR_SECONDS=120, SENSOR_STALE_MS=7000, SWITCH_MS=5000;
constexpr uint32_t SCHEMA=4;
// Credencial de ejemplo para el repositorio; cambiar antes de cargar al equipo.
constexpr char INITIAL_PASSWORD[]="CAMBIA_ESTA_CLAVE";
enum class State : uint8_t { SIN_CICLO, INCUBANDO, PAUSADO, NACIMIENTO, FINALIZADO, ERROR_CRITICO };
enum class Mode : uint8_t { AUTOMATICO, MANUAL_TEMPORAL, DESHABILITADO, BLOQUEADO_POR_SEGURIDAD };
enum class IdlePolicy : uint8_t { OFF, REGULATE };
struct Stage { float heatOn=37.5f, heatOff=37.8f, fanOn=38.0f, fanOff=37.8f, humidOn=50, humidOff=60; };
struct Config {
  uint32_t schema=SCHEMA;
  Stage incubation, birth{37.2f,37.5f,37.8f,37.5f,65,70};
  uint32_t motorSeconds=10, motorMaxSeconds=30, intervalSeconds=10800, firstMovement=0;
  float overtemp=39.5f;
  bool automaticTurn=true, finishAt18=false, activeLow[4]={false,false,false,false};
  bool hardwareConfirmed=false, emergencyFan=false;
  IdlePolicy idlePolicy=IdlePolicy::OFF;
  uint8_t lcdAddress=0x27;
  char apPassword[64]={0}, adminPassword[64]={0};
};
struct Cycle {
  uint32_t schema=SCHEMA;
  State state=State::SIN_CICLO;
  uint32_t startedAt=0, resumedAt=0, elapsedSaved=0, pauseStarted=0, pauseTotal=0;
  uint32_t nextTurnElapsed=0, lastAttempt=0, lastConfirmed=0;
  bool motorPending=false;
};
inline bool running(State s) { return s==State::INCUBANDO || s==State::NACIMIENTO; }
inline bool existing(State s) { return running(s) || s==State::PAUSADO; }
inline uint32_t elapsed(const Cycle& c,uint32_t now) {
  return c.elapsedSaved+(running(c.state) && now>=c.resumedAt ? now-c.resumedAt : 0);
}
inline uint32_t endSeconds(const Config& c) { return c.finishAt18?HATCH:END; }
inline State stageAt(uint32_t e,const Config& c) { return e>=endSeconds(c)?State::FINALIZADO:(e>=HATCH?State::NACIMIENTO:State::INCUBANDO); }
inline uint32_t futureSlot(uint32_t slot,uint32_t e,uint32_t interval) { return slot>e?slot:slot+((e-slot)/interval+1)*interval; }
inline bool stageValid(const Stage& s) {
  return isfinite(s.heatOn)&&isfinite(s.heatOff)&&isfinite(s.fanOn)&&isfinite(s.fanOff)&&isfinite(s.humidOn)&&isfinite(s.humidOff)
    && s.heatOn>=30 && s.heatOff<=39 && s.heatOn<s.heatOff
    && s.fanOff>=s.heatOff && s.fanOn>s.fanOff && s.fanOn<=40
    && s.humidOn>=20 && s.humidOff<=85 && s.humidOn<s.humidOff;
}
inline bool valid(const Config& c) {
  return c.schema==SCHEMA && stageValid(c.incubation)&&stageValid(c.birth)
    && c.motorSeconds>=1 && c.motorSeconds<=c.motorMaxSeconds && c.motorMaxSeconds<=ABS_MOTOR_SECONDS
    && c.intervalSeconds>=600 && c.intervalSeconds<=DAY && c.intervalSeconds>c.motorMaxSeconds
    && isfinite(c.overtemp) && c.overtemp>c.incubation.fanOn && c.overtemp>c.birth.fanOn && c.overtemp<=42
    && (c.idlePolicy==IdlePolicy::OFF || c.idlePolicy==IdlePolicy::REGULATE)
    && (c.lcdAddress==0x27 || c.lcdAddress==0x3f);
}
inline bool plausible(float t,float h) { return isfinite(t)&&isfinite(h)&&t>=0&&t<=60&&h>=1&&h<=100; }
struct Demand { bool heat,fan,humid; };
inline Demand regulate(const Stage& s,float t,float h,Demand prev) {
  bool f=t>s.fanOn || (prev.fan && t>s.fanOff);
  bool heat=!f && (t<s.heatOn || (prev.heat && t<s.heatOff));
  return {heat,f,h<s.humidOn || (prev.humid && h<s.humidOff)};
}
}

#include "storage.h"
#include <stddef.h>
namespace paulett {
static uint32_t checksum(const uint8_t* p,size_t n) { uint32_t h=2166136261UL; while(n--) h=(h^*p++)*16777619UL; return h; }
template<class T> struct Record { T value; uint32_t crc; };
template<class T> bool read(Preferences& p,const char* key,T& out) {
 Record<T> r{}; if(p.getBytesLength(key)!=sizeof(r)||p.getBytes(key,&r,sizeof(r))!=sizeof(r)) return false;
 if(r.crc!=checksum(reinterpret_cast<uint8_t*>(&r.value),sizeof(T))) return false;
 out=r.value; return out.schema==SCHEMA;
}
template<class T> bool write(Preferences& p,const char* key,const T& v) {
 Record<T> r{}; r.value=v; r.crc=checksum(reinterpret_cast<const uint8_t*>(&r.value),sizeof(T));
 return p.putBytes(key,&r,sizeof(r))==sizeof(r);
}
bool Storage::begin() {
#ifdef PAULETT_SIMULATION
 return prefs.begin("paulett-test",false);
#else
 return prefs.begin("paulett",false);
#endif
}
bool Storage::load(Config& cfg,Cycle& cycle,bool& fresh) {
 fresh=!prefs.isKey("config")&&!prefs.isKey("cycle");
 if(fresh) return true;
 return read(prefs,"config",cfg)&&valid(cfg)&&read(prefs,"cycle",cycle)
   && static_cast<uint8_t>(cycle.state)<=static_cast<uint8_t>(State::ERROR_CRITICO)
   && cycle.elapsedSaved<=END && cycle.nextTurnElapsed<=END+DAY
   && memchr(cfg.apPassword,0,sizeof(cfg.apPassword)) && strlen(cfg.apPassword)>=8
   && memchr(cfg.adminPassword,0,sizeof(cfg.adminPassword)) && strlen(cfg.adminPassword)>=8;
}
bool Storage::applyCredentialPreset(Config& cfg) {
 // Migracion unica: actualiza claves existentes sin borrar ciclo ni parametros.
 // Guardar primero el registro; marcar al final para reintentar si se corta la energia.
 if(prefs.getBool("credsPreset1",false)) return true;
 Config updated=cfg;
 strlcpy(updated.apPassword,INITIAL_PASSWORD,sizeof(updated.apPassword));
 strlcpy(updated.adminPassword,INITIAL_PASSWORD,sizeof(updated.adminPassword));
 if(!saveConfig(updated)) return false;
 cfg=updated;
 return prefs.putBool("credsPreset1",true)==1;
}
bool Storage::applyActiveHighPreset(Config& cfg) {
 // Migrar una sola vez antes de configurar GPIO; conservar ciclo y otros ajustes.
 if(prefs.getBool("relaysHigh1",false)) return true;
 for(bool& activeLow:cfg.activeLow) activeLow=false;
 if(!saveConfig(cfg)) return false;
 return prefs.putBool("relaysHigh1",true)==1;
}
bool Storage::saveConfig(const Config& v) { return write(prefs,"config",v); }
bool Storage::saveCycle(const Cycle& v) { return write(prefs,"cycle",v); }
}

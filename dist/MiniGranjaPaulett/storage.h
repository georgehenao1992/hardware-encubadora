#pragma once
#include <Preferences.h>
#include "config.h"
namespace paulett {
class Storage {
 Preferences prefs;
 public:
 bool begin();
 bool load(Config&,Cycle&,bool& fresh);
 bool saveConfig(const Config&);
 bool applyCredentialPreset(Config&);
 bool applyActiveHighPreset(Config&);
 bool saveCycle(const Cycle&);
};
}

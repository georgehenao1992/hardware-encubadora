#pragma once
#include <LiquidCrystal_I2C.h>
#include "lcd_wake.h"
namespace paulett {
class Lcd {
 LiquidCrystal_I2C* lcd=nullptr; String previous[4]; uint32_t updated=0;
 LcdWake wake; bool visible=false;
 public: void begin(); void tick();
};
extern Lcd display;
}

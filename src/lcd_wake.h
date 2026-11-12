#pragma once
#include <stdint.h>
namespace paulett {
constexpr uint8_t LCD_BUTTON_PIN = 32;
// Temporizador independiente del RTC y del ciclo de incubacion.
class LcdWake {
  bool rawPressed=false, stablePressed=false, awake=false;
  uint32_t changedAt=0, pressedAt=0;
 public:
  bool tick(bool pressed, uint32_t now) {
    if (pressed != rawPressed) { rawPressed=pressed; changedAt=now; }
    if (rawPressed != stablePressed && uint32_t(now-changedAt)>=40) {
      stablePressed=rawPressed;
      if (stablePressed) { awake=true; pressedAt=now; }
    }
    if (awake && uint32_t(now-pressedAt)>=30000) awake=false;
    return awake;
  }
};
}

#include "lcd_manager.h"
#include "app.h"
#include <Wire.h>

namespace paulett {
Lcd display;

namespace {
// La foto del equipo confirma 20 columnas: filas 3/4 en 0x14 y 0x54.
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t ROW_ADDRESS[4] = {0x00, 0x40, 0x14, 0x54};

String eventText(const String& code) {
  if (code == "HUMIDIFICADOR ON") return "HUMIDIFICADOR ON";
  if (code == "HUMIDIFICADOR OFF") return "HUMIDIF. OFF";
  if (code == "CICLO REANUDADO") return "CICLO REANUDADO";
  if (code == "CAMBIO DIA") {
    return "INICIA DIA " + String(min(app.logicalElapsed() / DAY + 1, uint32_t(21)));
  }
  if (code == "PRUEBA RELE") return "PRUEBA DE RELE";
  return code.substring(0, 16);
}
}

void Lcd::begin() {
  pinMode(LCD_BUTTON_PIN, INPUT_PULLUP);
  Wire.beginTransmission(app.config.lcdAddress);
  if (Wire.endTransmission() != 0) {
    app.event("LCD AUSENTE", "Revisar direccion, cableado y niveles I2C");
    return;
  }
  lcd = new LiquidCrystal_I2C(app.config.lcdAddress, LCD_COLUMNS, 4);
  lcd->init();
  lcd->noBacklight();
  lcd->noDisplay();
  // Esperar al pulsador; el primer encendido dibuja las cuatro filas actuales.
  updated = millis() - 500;
  tick();
}

void Lcd::tick() {
  const uint32_t ms = millis();
  if (!lcd) return;
  const bool requested = wake.tick(digitalRead(LCD_BUTTON_PIN) == LOW, ms);
  const bool waking = requested && !visible;
  if (!requested && visible) { lcd->noBacklight(); lcd->noDisplay(); }
  visible = requested;
  if (!visible) return;
  if (!waking && ms - updated < 500) return;
  updated = ms;

  String lines[4];
  lines[0] = "GRANJA PAULETT";
  const bool sensorOK = app.sensors.reliable(ms);
  // Conservar siempre las etiquetas y filas; no presentar datos viejos como actuales.
  lines[1] = sensorOK ? "TEMP: " + String(app.sensors.temperature, 1) + " C" : "TEMP: --.- C";
  lines[2] = sensorOK ? "HUM : " + String(app.sensors.humidity, 1) + " %" : "HUM : --.- %";

  // La fecha es la vista de reposo. Los eventos la sustituyen temporalmente.
  if (app.clock.valid && !app.clock.jumped) {
    const DateTime dt(app.clock.now);
    char date[24];
    snprintf(date, sizeof(date), "%02u:%02u %02u/%02u/%02u",
             unsigned(dt.hour()), unsigned(dt.minute()), unsigned(dt.day()),
             unsigned(dt.month()), unsigned(dt.year() % 100));
    lines[3] = date;
  } else {
    lines[3] = "ERROR RTC";
  }

  if (int32_t(app.lcdEventUntil - ms) > 0) {
    lines[3] = eventText(app.lcdEvent);
  } else if ((ms / 1000) % 10 < 5) {
    // Alarmas persistentes y finalizacion alternan con la fecha, no la ocultan siempre.
    if (!app.storageOK || app.cycle.state == State::ERROR_CRITICO) lines[3] = "ERROR CRITICO";
    else if (!sensorOK) lines[3] = "ERROR SENSOR";
    else if (app.cycle.state == State::FINALIZADO) lines[3] = "CICLO TERMINADO";
  }

  for (uint8_t row = 0; row < 4; row++) {
    lines[row] = lines[row].substring(0, LCD_COLUMNS);
    while (lines[row].length() < LCD_COLUMNS) lines[row] += ' ';
    if (previous[row] == lines[row]) continue;
    lcd->command(LCD_SETDDRAMADDR | ROW_ADDRESS[row]);
    lcd->print(lines[row]);
    previous[row] = lines[row];
  }
  // Actualizar primero el contenido para no mostrar lecturas viejas al despertar.
  if (waking) { lcd->display(); lcd->backlight(); }
}
}

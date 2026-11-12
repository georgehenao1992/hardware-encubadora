#include <Arduino.h>
#include <esp_task_wdt.h>
#include "app.h"
#include "lcd_manager.h"
#include "web_server.h"
using namespace paulett;
static void controlTask(void*) {
 esp_task_wdt_add(nullptr);
 TickType_t wake=xTaskGetTickCount();
 for(;;) {
  xSemaphoreTake(appMutex,portMAX_DELAY);
  app.tick(); display.tick();
  xSemaphoreGive(appMutex);
  esp_task_wdt_reset();
  vTaskDelayUntil(&wake,pdMS_TO_TICKS(10));
 }
}
void setup() {
 Serial.begin(115200);
 appMutex=xSemaphoreCreateMutex();
 if(!appMutex) { ESP.restart(); return; }
 app.begin(); display.begin();
 esp_task_wdt_config_t watchdog{};
 watchdog.timeout_ms=8000; watchdog.idle_core_mask=(1U<<portNUM_PROCESSORS)-1; watchdog.trigger_panic=true;
 if(esp_task_wdt_init(&watchdog)==ESP_ERR_INVALID_STATE) esp_task_wdt_reconfigure(&watchdog);
 if(xTaskCreatePinnedToCore(controlTask,"incubator",8192,nullptr,2,nullptr,1)!=pdPASS) { app.relays.allOff(); ESP.restart(); return; }
 webBegin();
}
void loop() {
 // Recuperacion local de credenciales: requiere acceso fisico al puerto USB.
 static String input;
 while(Serial.available()) {
  char c=Serial.read();
  if(c=='\n') {
   input.trim();
   if(input=="credenciales") {
    xSemaphoreTake(appMutex,portMAX_DELAY);
    String wifi=app.config.apPassword,admin=app.config.adminPassword;
    xSemaphoreGive(appMutex);
    Serial.printf("WiFi: %s\nAdministrador admin: %s\n",wifi.c_str(),admin.c_str());
   }
   if(input=="rtc") {
    xSemaphoreTake(appMutex,portMAX_DELAY);
    String report=app.clock.diagnostic()+" ciclo="+app.stateName();
    xSemaphoreGive(appMutex);
    Serial.println(report);
   }
   if(input=="estado") {
    DynamicJsonDocument snapshot(24576);
    xSemaphoreTake(appMutex,portMAX_DELAY);
    app.status(snapshot);
    xSemaphoreGive(appMutex);
    Serial.print("ESTADO "); serializeJson(snapshot,Serial); Serial.println();
   }
   if(input=="confirmar instalacion") {
    // Confirmacion fisica por USB: usa las mismas validaciones que el panel.
    DynamicJsonDocument command(4096);
    String error;
    xSemaphoreTake(appMutex,portMAX_DELAY);
    app.settings(command);
    command["action"]="config";
    command["confirm"]=true;
    command["hardwareConfirmed"]=true;
    bool ok=app.command(command,error);
    xSemaphoreGive(appMutex);
    Serial.println(ok?"INSTALACION CONFIRMADA":"INSTALACION RECHAZADA: "+error);
   }
   if(input.startsWith("hora:")) {
    String value=input.substring(5); bool digits=value.length()==10;
    for(char digit:value) if(digit<'0'||digit>'9') digits=false;
    if(digits) {
     StaticJsonDocument<192> command;
     command["action"]="clock"; command["confirm"]=true;
     command["epoch"]=uint32_t(strtoul(value.c_str(),nullptr,10));
     String error;
     xSemaphoreTake(appMutex,portMAX_DELAY);
     bool ok=app.command(command,error);
     xSemaphoreGive(appMutex);
     Serial.println(ok?"AJUSTE USB OK":"AJUSTE USB ERROR: "+error);
    } else Serial.println("Formato: hora:epoch_local_de_10_digitos");
   }
   input="";
  } else if(input.length()<40) input+=c;
 }
 webTick(); yield();
}

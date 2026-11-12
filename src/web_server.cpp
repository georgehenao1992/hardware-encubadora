#include "web_server.h"
#include "app.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
namespace paulett {
static WebServer server(80);
static uint32_t authWindow=0; static uint8_t failures=0;
struct Lock { Lock(){xSemaphoreTake(appMutex,portMAX_DELAY);} ~Lock(){xSemaphoreGive(appMutex);} };
static void headers() {
 server.sendHeader("Cache-Control","no-store"); server.sendHeader("X-Content-Type-Options","nosniff");
 server.sendHeader("X-Frame-Options","DENY"); server.sendHeader("Referrer-Policy","no-referrer");
 server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
}
static bool authorized() {
 headers();
 if(server.hostHeader()!="192.168.4.1" && server.hostHeader()!="192.168.4.1:80") { server.send(403,"application/json","{\"error\":\"Host no permitido\"}"); return false; }
 if(millis()-authWindow>=60000) { authWindow=millis(); failures=0; }
 if(failures>=10) { server.send(429,"application/json","{\"error\":\"Espere un minuto antes de intentar de nuevo\"}"); return false; }
 String password; { Lock lock; password=app.config.adminPassword; }
 if(!server.authenticate("admin",password.c_str())) { failures++; server.send(401,"application/json","{\"error\":\"Credenciales invalidas\"}"); return false; }
 return true;
}
static void jsonError(int code,const String& message) { StaticJsonDocument<256> d; d["error"]=message; String out; serializeJson(d,out); server.send(code,"application/json",out); }
void webBegin() {
 bool mounted=LittleFS.begin(false); { Lock lock; app.fsOK=mounted; if(!mounted) app.event("ERROR LITTLEFS","Cargar data con uploadfs; control autonomo disponible"); }
 WiFi.mode(WIFI_AP); WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0));
 String password; { Lock lock; password=app.config.apPassword; }
 if(!WiFi.softAP("MINI_GRANJA_PAULETT",password.c_str(),1,false,4)) { Lock lock; app.event("ERROR WIFI","No fue posible iniciar Access Point"); }
 const char* keys[]={"X-Paulett-Request","Content-Type"}; server.collectHeaders(keys,2);
 server.on("/api/status",HTTP_GET,[](){ if(!authorized()) return; DynamicJsonDocument doc(24576); { Lock lock; app.status(doc); } String out; serializeJson(doc,out); server.send(200,"application/json",out); });
 server.on("/api/config",HTTP_GET,[](){ if(!authorized()) return; DynamicJsonDocument doc(4096); { Lock lock; app.settings(doc); } String out; serializeJson(doc,out); server.send(200,"application/json",out); });
 server.on("/api/command",HTTP_POST,[](){
  if(!authorized()) return;
  if(server.header("X-Paulett-Request")!="1"||server.header("Content-Type")!="application/json") { jsonError(403,"Solicitud no permitida"); return; }
  String body=server.arg("plain"); if(body.length()>4096) { jsonError(413,"Solicitud demasiado grande"); return; }
  DynamicJsonDocument doc(6144); if(deserializeJson(doc,body)||!doc.is<JsonObject>()) { jsonError(400,"JSON invalido"); return; }
  String error; bool ok; { Lock lock; ok=app.command(doc,error); }
  if(!ok) jsonError(409,error); else server.send(200,"application/json","{\"ok\":true}");
 });
 server.onNotFound([](){
  headers(); String uri=server.uri(); const char* path=nullptr; const char* type=nullptr;
  if(uri=="/") { path="/index.html"; type="text/html; charset=utf-8"; }
  else if(uri=="/styles.css") { path="/styles.css"; type="text/css"; }
  else if(uri=="/app.js") { path="/app.js"; type="application/javascript"; }
  if(server.method()!=HTTP_GET||!path) { server.send(404,"text/plain","No encontrado"); return; }
  File file=LittleFS.open(path,"r"); if(!file) { server.send(503,"text/plain","MINI GRANJA PAULETT: cargue LittleFS con pio run -t uploadfs"); return; }
  server.streamFile(file,type); file.close();
 });
 server.begin();
}
void webTick() { server.handleClient(); }
}

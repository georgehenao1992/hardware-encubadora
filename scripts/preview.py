#!/usr/bin/env python3
"""Vista de diseño con datos ficticios, solo localhost; no controla hardware."""
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
STAGE=dict(heatOn=37.5,heatOff=37.8,fanOn=38,fanOff=37.8,humidOn=50,humidOff=60)
CONFIG=dict(incubation=STAGE,birth=dict(heatOn=37.2,heatOff=37.5,fanOn=37.8,fanOff=37.5,humidOn=65,humidOff=70),motorSeconds=10,motorMaxSeconds=30,intervalSeconds=10800,firstMovement=0,automaticTurn=True,finishAt18=False,overtemp=39.5,emergencyFan=False,idlePolicy=0,hardwareConfirmed=True,lcdAddress=39,activeLow=[False]*4)
NOW=1793311800
STATUS=dict(state='INCUBANDO',elapsed=7*86400+18000,day=8,remaining=14*86400-18000,temperature=37.7,humidity=56.3,sensorOK=True,sensorAge=400,rtcOK=True,now=NOW,storageOK=True,hardwareConfirmed=True,fsOK=True,idlePolicy=0,emergencyFan=False,lastAttempt=NOW-5000,lastConfirmed=NOW-4990,nextTurn=NOW+5800,simulation=True,alarms=['VISTA DE DISEÑO · Datos ficticios. No hay conexión con una ESP32.'],relays=[dict(name=n,on=i==1,mode='AUTOMATICO') for i,n in enumerate(['MOTOR','CALEFACCION','VENTILADOR','HUMIDIFICADOR'])],events=[dict(time=NOW-i*120,code=c,description=d) for i,(c,d) in enumerate([('CALEFACCION ON','Control de temperatura activo'),('MOTOR APAGADO','Movimiento completado'),('CAMBIO DIA','Día 8 del ciclo'),('RELOJ OK','Referencia temporal válida')])])
class Handler(SimpleHTTPRequestHandler):
    def __init__(self,*args,**kwargs):super().__init__(*args,directory=str(ROOT/'data'),**kwargs)
    def do_GET(self):
        if self.path in ('/api/status','/api/config'):
            data=json.dumps(STATUS if self.path.endswith('status') else CONFIG).encode()
            self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
        else:super().do_GET()
    def do_POST(self):
        self.send_response(409);self.send_header('Content-Type','application/json');self.end_headers();self.wfile.write(b'{"error":"Vista de diseno: las acciones estan deshabilitadas"}')
if __name__=='__main__':
    print('Vista ficticia http://127.0.0.1:8080 — cualquier clave de 8 caracteres',flush=True)
    ThreadingHTTPServer(('127.0.0.1',8080),Handler).serve_forever()

#!/usr/bin/env python3
"""Diagnostica el reloj; --set-time copia la hora local de Colombia por USB."""
import argparse
import calendar
from datetime import datetime
import time
from zoneinfo import ZoneInfo
import serial

parser=argparse.ArgumentParser()
parser.add_argument('--port',default='/dev/cu.usbserial-57670563791')
parser.add_argument('--set-time',action='store_true')
args=parser.parse_args()
port=serial.Serial()
port.port=args.port
port.baudrate=115200
port.timeout=0.2
port.dtr=False
port.rts=False
port.open()

def collect(seconds):
    end=time.monotonic()+seconds
    reports=[]
    while time.monotonic()<end:
        line=port.readline().decode('utf-8',errors='replace').strip()
        if line.startswith('RTC presente=') or 'AJUSTE USB' in line or 'RELOJ OK' in line:
            print(line,flush=True)
            reports.append(line)
    return reports

try:
    collect(4)  # Algunos adaptadores reinician al abrir el puerto.
    port.write(b'rtc\n')
    reports=collect(2)
    if not any(line.startswith('RTC presente=') for line in reports):
        raise SystemExit('No hubo respuesta al diagnostico; verificar firmware y puerto.')
    if args.set_time:
        local=datetime.now(ZoneInfo('America/Bogota'))
        epoch=calendar.timegm(local.timetuple())  # El firmware almacena componentes locales.
        print('Ajustando hora local:',local.strftime('%d/%m/%Y %H:%M:%S'),flush=True)
        port.write(f'hora:{epoch}\n'.encode())
        result=collect(3)
        if not any(line=='AJUSTE USB OK' for line in result):
            raise SystemExit('El ajuste no fue confirmado; no se reanuda ningun ciclo.')
    for _ in range(3):
        port.write(b'rtc\n')
        collect(3)
finally:
    port.close()

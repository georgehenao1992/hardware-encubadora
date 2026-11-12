"""Infografía vectorial A3 basada en el firmware; no modifica el dispositivo."""
from pathlib import Path
import sys
sys.path.insert(0, '/private/tmp/paulett_pdf_deps')
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import Paragraph
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.pagesizes import A3
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output/pdf/Infografia_logica_MVP_Paulett.pdf'
OUT.parent.mkdir(parents=True,exist_ok=True)
for name,file in [('R','Arial.ttf'),('B','Arial Bold.ttf')]:
    pdfmetrics.registerFont(TTFont(name,'/System/Library/Fonts/Supplemental/'+file))
c=canvas.Canvas(str(OUT),pagesize=A3)
c.setTitle('Mini Granja Paulett | Lógica del MVP')
c.setAuthor('Mini Granja Paulett')
c.scale(A3[0]/1000,A3[1]/1414)
W,H=1000,1414
ink='#173E39'; green='#18725E'; mint='#E8F3EC'; bg='#F7F8F2'; gray='#536760'; blue='#237F99'; amber='#B67321'; red='#AE493A'
def box(x,y,w,h,fill,r=12,stroke=None):
    c.setFillColor(HexColor(fill))
    c.setStrokeColor(HexColor(stroke or fill))
    c.roundRect(x,H-y-h,w,h,r,fill=1,stroke=bool(stroke))
def txt(x,y,s,size=13,color=ink,bold=False):
    c.setFillColor(HexColor(color)); c.setFont('B' if bold else 'R',size); c.drawString(x,H-y-size,s)
def para(x,y,w,s,size=13,color=ink,limit=100):
    p=Paragraph(s,ParagraphStyle('p',fontName='R',fontSize=size,leading=size*1.32,textColor=HexColor(color)))
    _,h=p.wrap(w,1000)
    assert h<=limit,(s,h,limit)
    p.drawOn(c,x,H-y-h)
def line(x,y,x2,y2,color=green,width=2):
    c.setStrokeColor(HexColor(color)); c.setLineWidth(width); c.line(x,H-y,x2,H-y2)
def arrow(x,y,x2,y2,color=green):
    line(x,y,x2,y2,color)
    import math
    a=math.atan2(y2-y,x2-x)
    for da in [-.5,.5]: line(x2,y2,x2-8*math.cos(a+da),y2-8*math.sin(a+da),color)
def section(y,n,title,sub=None):
    box(36,y,29,29,green,8); txt(43,y+4,n,16,'#FFFFFF',True)
    txt(76,y+1,title,21,ink,True)
    if sub: txt(76,y+30,sub,12,gray)
box(0,0,W,H,bg,0)
box(0,0,W,143,ink,0)
txt(36,23,'MINI GRANJA PAULETT',14,'#C8E7D6',True)
txt(36,48,'Así funciona la lógica del MVP',35,'#FFFFFF',True)
txt(36,99,'Leer  >  validar  >  decidir  >  actuar  >  informar',17,'#D7EADF')
box(788,24,176,28,'#315A50',7); txt(800,30,'SOPORTE DE DESARROLLO',10,'#FFFFFF',True)
txt(804,65,'ESP32 · control local',13,'#FFFFFF')
txt(804,88,'Versión documental 1.0',11,'#C8E7D6')
txt(804,108,'04 NOV 2026',11,'#C8E7D6')
section(160,'01','De las mediciones a las salidas')
xs=[36,274,512,750]
labels=[('ENTRADAS','DHT22: temperatura y humedad.<br/>DS3231: fecha y hora.<br/>App: parámetros del usuario.'),('VALIDACIÓN','Sensor, reloj y memoria.<br/>Instalación confirmada.<br/>Etapa y límites de seguridad.'),('DECISIÓN · ESP32','Histéresis ambiental.<br/>Temporización del volteo.<br/>Prioridad de ventilación.'),('SALIDAS','Motor · Calefacción<br/>Ventilador · Humidificador<br/>Estado en app y LCD.')]
for x,(a,b) in zip(xs,labels):
    box(x,202,214,111,'#FFFFFF',10, '#DCE6DE'); txt(x+13,215,a,13,green,True); para(x+13,241,189,b,12,limit=62)
for x in [250,488,726]: arrow(x+3,256,x+20,256)
txt(37,321,'Control cada 10 ms · DHT22 cada 2,5 s · El panel web no sustituye el control autónomo.',12,gray)
section(352,'02','Histéresis: el estado anterior también decide','Entre ON y OFF se conserva el estado previo; estar dentro del rango no obliga a encender.')
# Three parallel hysteresis diagrams, with actual strict comparisons.
items=[(36,'CALEFACCIÓN','Temperatura',32,35.5,'32 °C','35,5 °C',amber,'T < 32 °C','T ≥ 35,5 °C',False),
       (351,'HUMIDIFICADOR','Humedad relativa',50,60,'50 %','60 %',blue,'HR < 50 %','HR ≥ 60 %',False),
       (666,'VENTILADOR','Temperatura',35.5,36,'35,5 °C','36 °C',green,'T > 36 °C','T ≤ 35,5 °C',True)]
for x,title,axis,lo,hi,ls,hs,col,on,off,fan in items:
    box(x,414,298,223,'#FFFFFF',12,'#DCE6DE')
    txt(x+16,427,title,15,col,True)
    txt(x+16,451,axis+' creciente →',11,gray)
    box(x+93,479,111,39,mint,0)
    line(x+22,498,x+275,498,gray,1.5)
    for a in [93,204]: line(x+a,476,x+a,521,col,2)
    txt(x+93,482,'MEMORIA',11,green,True)
    txt(x+66,524,ls,12,col,True); txt(x+178,524,hs,12,col,True)
    txt(x+20,563,'ENCIENDE: '+on,14,col,True)
    txt(x+20,589,'APAGA: '+off,14,ink,True)
box(36,650,928,64,mint,10)
para(51,660,899,'<b>Ejemplo: 32,6 °C y 55 % HR.</b> Si calefacción y humidificador estaban apagados, siguen apagados. Si estaban encendidos, permanecen encendidos hasta sus umbrales OFF, salvo una protección o la prioridad del ventilador.',13,limit=44)
txt(37,725,'REGLA DE CONFIGURACIÓN',11,green,True)
txt(221,723,'Calefacción ON < OFF ≤ Ventilador OFF < ON  |  Humedad ON < OFF',13,ink,True)
txt(37,746,'Valores didácticos, no lecturas reales ni un protocolo biológico. A igualdad con ON no arranca desde OFF.',12,gray)
section(776,'03','El ciclo organiza el tiempo y el volteo')
# Cycle progression, blocks and arrows.
for x,w,label,detail in [(36,155,'SIN CICLO','Reposo configurable'),(218,268,'INCUBANDO','0 a menos de 432 h · volteo habilitable'),(513,245,'NACIMIENTO','432 a menos de 504 h · sin volteo'),(785,179,'FINALIZADO','504 h · fin del ciclo')]:
    box(x,820,w,64,ink if x==218 else mint,8)
    txt(x+12,829,label,13,'#FFFFFF' if x==218 else green,True)
    para(x+12,852,w-22,detail,10,'#FFFFFF' if x==218 else ink,limit=28)
for a,b in [(191,218),(486,513),(758,785)]: arrow(a+3,850,b-4,850)
para(36,895,445,'<b>PAUSADO:</b> congela el tiempo del ciclo y detiene el motor. El control de temperatura y humedad continúa sujeto a las protecciones.',12,limit=50)
para(512,895,452,'<b>Volteo:</b> duración e intervalo configurables; corte temporizado y límite absoluto de 120 s. No recupera movimientos perdidos tras un reinicio.',12,limit=50)
txt(37,949,'432 h = 18 días completos (inicio del día 19). Opción de cierre temprano: finalizar a las 432 h.',11,gray)
section(977,'04','Las protecciones tienen prioridad')
for x,y,title,body in [(36,1020,'SENSOR INVÁLIDO','Apaga calefacción, humedad y motor. Ventilador según la política de emergencia.'),(512,1020,'SOBRETEMPERATURA','Apaga calefacción, humedad y motor; solicita ventilación si la instalación está habilitada.'),(36,1095,'RELOJ INVÁLIDO O CON SALTO','Pausa el ciclo y detiene el motor. Requiere corregir el reloj y reanudar manualmente.'),(512,1095,'MEMORIA O INSTALACIÓN','Fallo de NVS o instalación sin confirmar: salidas bloqueadas. Ventilador y calor no operan juntos.')]:
    box(x,y,452,65,'#FFFFFF',8,'#E3E7DF'); box(x,y,5,65,red,2)
    txt(x+15,y+8,title,12,red,True); para(x+15,y+28,423,body,11,limit=32)
section(1178,'05','Interfaz física y ahorro de pantalla')
box(36,1220,452,94,ink,10)
txt(52,1233,'GPIO32  →  PULSADOR N.A.  →  GND',17,'#FFFFFF',True)
para(52,1264,420,'Pantalla apagada al iniciar. Pulsar: luz y datos por <b>30 s</b>. Otra pulsación reinicia el plazo. El control sigue activo.',12,'#FFFFFF',limit=40)
box(512,1220,452,94,mint,10)
txt(528,1233,'4 RELÉS: HIGH = ON · LOW = OFF',16,green,True)
para(528,1264,420,'GPIO25 motor · GPIO26 calefacción<br/>GPIO27 ventilador · GPIO33 humidificador<br/>LCD: título / temperatura / humedad / fecha y eventos.',11,limit=45)
line(36,1330,964,1330,'#CFDCD2',1)
para(36,1340,928,'<b>MVP:</b> producto mínimo viable · <b>Histéresis:</b> dos umbrales y memoria de estado · <b>RTC:</b> reloj en tiempo real<br/><b>GPIO:</b> pin de entrada/salida · <b>NVS:</b> memoria persistente · <b>HR:</b> humedad relativa · <b>N.A.:</b> normalmente abierto',10,gray,limit=30)
para(36,1380,928,'Base: src/config.h, app.cpp, relays.cpp, lcd_manager.cpp y lcd_wake.h. Describe el firmware; no certifica el montaje eléctrico ni mide el ahorro energético.',9,gray,limit=24)
c.showPage(); c.save()
print(OUT)

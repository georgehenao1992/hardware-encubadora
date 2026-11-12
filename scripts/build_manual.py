#!/usr/bin/env python3
"""Manual visual versionado. Solo genera documentación; no controla la ESP32."""
from pathlib import Path
import sys, math
sys.path.insert(0,'/private/tmp/paulett_pdf_deps')
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, Color, white
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import Paragraph
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output/pdf/Manual_visual_Mini_Granja_Paulett.pdf'
OUT.parent.mkdir(parents=True,exist_ok=True)
fonts=Path('/System/Library/Fonts/Supplemental')
for name,file in [('Sans','Arial.ttf'),('Bold','Arial Bold.ttf'),('Serif','Georgia.ttf')]:pdfmetrics.registerFont(TTFont(name,str(fonts/file)))
pdfmetrics.registerFontFamily('Sans',normal='Sans',bold='Bold',italic='Sans',boldItalic='Bold')
W,H=595.276,841.89
M=42; CW=W-2*M
INK='#193D33'; MUTED='#586B63'; GREEN='#287450'; LIGHT='#EAF3E9'; PAPER='#FAFBF7'; LINE='#D9E2D9'; ORANGE='#AE572D'; PALE='#FFF0DF'; BLUE='#2D6E97'; RED='#A63F38'; GRAY='#EEF1ED'
c=canvas.Canvas(str(OUT),pagesize=(W,H),pageCompression=1)
c.setTitle('Manual visual del sistema de control MVP - Mini Granja Paulett')
c.setAuthor('MINI GRANJA PAULETT')
c.setSubject('Histéresis, operación, configuración y diagnóstico del firmware del proyecto. Revisión 02/11/2026.')
page=0

def rect(x,y,w,h,fill=PAPER,stroke=None,r=10):
 c.setFillColor(HexColor(fill));c.setStrokeColor(HexColor(stroke or fill));c.roundRect(x,H-y-h,w,h,r,fill=1,stroke=bool(stroke))
def text(s,x,y,size=11,font='Sans',color=INK):
 c.setFillColor(HexColor(color));c.setFont(font,size);c.drawString(x,H-y-size,s)
def para(s,x,y,w,size=11,color=INK,bold=False,maxh=None):
 st=ParagraphStyle('body',fontName='Bold' if bold else 'Sans',fontSize=size,leading=size*1.42,textColor=HexColor(color),spaceAfter=0)
 p=Paragraph(s,st);ww,hh=p.wrap(w,1000)
 if maxh is not None: assert hh<=maxh,(page,s[:75],hh,maxh)
 assert y+hh<790,(page,'overflow',s[:75],y+hh)
 p.drawOn(c,x,H-y-hh);return hh

def line(x1,y1,x2,y2,color=LINE,width=1,dash=None):
 c.setStrokeColor(HexColor(color));c.setLineWidth(width);c.setDash(dash or []);c.line(x1,H-y1,x2,H-y2);c.setDash([])
def arrow(x1,y1,x2,y2,color=GREEN):
 line(x1,y1,x2,y2,color,1.7)
 a=math.atan2(y2-y1,x2-x1)
 for d in [-.5,.5]:line(x2,y2,x2-7*math.cos(a+d),y2-7*math.sin(a+d),color,1.7)
def tag(label,x,y,w=110,color=GREEN,bg=LIGHT):
 rect(x,y,w,25,bg,r=12);text(label,x+10,y+6,9,'Bold',color)
def begin(kicker,title,subtitle=''):
 global page
 if page:c.showPage()
 page+=1
 c.setFillColor(HexColor(PAPER));c.rect(0,0,W,H,fill=1,stroke=0)
 text('MINI GRANJA PAULETT',M,24,9,'Bold',GREEN)
 text('MANUAL VISUAL / MVP',W-170,24,8,'Sans',MUTED)
 line(M,46,W-M,46)
 text(kicker.upper(),M,65,9,'Bold',GREEN)
 text(title,M,85,26,'Serif')
 if subtitle:para(subtitle,M,127,CW,11,MUTED,maxh=47)
 line(M,794,W-M,794)
 text('Revisión 02/11/2026 · Uso local · Firmware del proyecto',M,807,8,'Sans',MUTED)
 text(f'{page:02d} / 15',W-79,807,8,'Bold',GREEN)
 c.bookmarkPage(f'p{page}')
 c.addOutlineEntry(title,f'p{page}',0)

def box(title,body,x,y,w,h,bg=LIGHT):
 rect(x,y,w,h,bg)
 para(title,x+16,y+13,w-32,12,bold=True,maxh=37)
 para(body,x+16,y+47,w-32,10.5,maxh=h-55)
def note(s,y,bg=PALE,h=65):
 rect(M,y,CW,h,bg)
 para(s,M+16,y+13,CW-32,10.5,maxh=h-20)
def table(headers,rows,y,widths=None,rowh=43,size=10):
 widths=widths or [CW/len(headers)]*len(headers)
 x=M
 for head,w in zip(headers,widths):
  rect(x,y,w,35,INK,r=0);para(head,x+10,y+9,w-20,9.5,'#FFFFFF',bold=True,maxh=28);x+=w
 for i,row in enumerate(rows):
  yy=y+35+i*rowh;rect(M,yy,CW,rowh,'#FFFFFF' if i%2==0 else GRAY,r=0)
  x=M
  for value,w in zip(row,widths):para(str(value),x+10,yy+10,w-20,size,maxh=rowh-14);x+=w
 return y+35+len(rows)*rowh

def bullet(title,body,y,n=None):
 if n is not None:
  rect(M,y,26,26,GREEN,r=13);text(str(n),M+9,y+5,11,'Bold','#FFFFFF');x=M+40
 else:x=M
 text(title,x,y,12,'Bold');hh=para(body,x,y+23,W-M-x,10.5,MUTED);return y+hh+40

def chart(values,on,off,kind,unit,y=206):
 # Muestras ilustrativas calculadas con las mismas desigualdades del firmware.
 states=[];state=False
 for v in values:
  state=(v>on or (state and v>off)) if kind=='fan' else (v<on or (state and v<off))
  states.append(state)
 x0=M+43;x1=W-M-16;top=y+27;bottom=y+205
 low=min(values+[on,off])-0.6 if unit=='°C' else min(values+[on,off])-3
 high=max(values+[on,off])+0.6 if unit=='°C' else max(values+[on,off])+3
 xx=lambda i:x0+(x1-x0)*i/(len(values)-1)
 yy=lambda v:bottom-(v-low)/(high-low)*(bottom-top)
 rect(M,y,CW,306,'#FFFFFF',LINE)
 a,b=sorted([yy(on),yy(off)])
 rect(x0,a,x1-x0,b-a,LIGHT,r=0)
 text(unit,M+12,y+6,10,'Bold',MUTED)
 for val,label,col in [(on,'ON',GREEN),(off,'OFF',ORANGE)]:
  line(x0,yy(val),x1,yy(val),col,1,[4,3]);text(f'{val:g}'.replace('.',','),M+7,yy(val)-6,9,'Bold',col)
  text(label,x1-25,yy(val)-14,8,'Bold',col)
 line(x0,top,x0,bottom,MUTED);line(x0,bottom,x1,bottom,MUTED)
 for i in range(len(values)-1):line(xx(i),yy(values[i]),xx(i+1),yy(values[i+1]),BLUE,2.2)
 for i,v in enumerate(values):
  c.setFillColor(HexColor(BLUE));c.circle(xx(i),H-yy(v),3,fill=1,stroke=0)
  text(str(v).replace('.',','),xx(i)-10,yy(v)-20,8,'Bold',BLUE)
  text(f't{i}',xx(i)-5,bottom+7,8,'Sans',MUTED)
 text('Estado tras cada lectura',M+14,y+241,9,'Bold')
 for i,s in enumerate(states):
  w=(x1-x0)/len(values)
  rect(x0+i*w,y+262,w-2,23,GREEN if s else GRAY,r=3)
  text(f't{i} '+('ON' if s else 'OFF'),x0+i*w+3,y+269,7.2,'Bold','#FFFFFF' if s else MUTED)
 return states

def glossary(items,y=181):
 colw=(CW-24)/2
 for col,entries in enumerate([items[:len(items)//2],items[len(items)//2:]]):
  yy=y;x=M+col*(colw+24)
  for title,body in entries:
   text(title,x,yy,11.5,'Bold',GREEN)
   h=para(body,x,yy+23,colw,10.5,maxh=80)
   yy+=max(83,h+38)

# 1
begin('Guía de operación · versión 1.0','Entiende tu incubadora','MINI GRANJA PAULETT · Manual del sistema de control MVP')
para('Qué enciende, qué apaga<br/>y por qué a veces espera.',M,190,335,24,bold=True,maxh=115)
para('Una guía visual para configurar temperatura, humedad y volteo, interpretar la app y reconocer las protecciones.',M,310,345,12,MUTED,maxh=75)
# Ilustración vectorial de incubadora.
rect(398,186,145,198,LIGHT,LINE,18);rect(411,203,119,50,INK,r=5)
text('PAULETT',423,214,12,'Bold','#FFFFFF');text('T 32,6   HR 55%',420,235,9,'Sans','#FFFFFF')
rect(411,267,119,100,'#FFFFFF',LINE,8)
for x in [431,471,510]:
 c.setFillColor(HexColor('#EBDCC1'));c.ellipse(x-12,H-346,x+12,H-305,stroke=0,fill=1)
line(419,351,522,351,GREEN,2)
box('La idea clave','Entre ON y OFF, el equipo conserva su estado anterior. OFF no es una meta que se persiga desde apagado.',M,420,CW,103)
text('Encuentra lo que necesitas',M,554,15,'Bold')
for i,(a,b) in enumerate([('Tu caso y el mapa del sistema','02-03'),('Gráficas de calefacción, humedad y ventilación','04-06'),('Cómo guardar parámetros y manejar el ciclo','07-09'),('Motor, relés y diagnóstico','10-12'),('Glosario y hoja rápida de consulta','13-15')]):
 yy=590+i*29;text(a,M,yy,10.5);text(b,W-M-39,yy,10.5,'Bold',GREEN)
para('Alcance: describe el software implementado al 02/11/2026. Los ejemplos de aprendizaje no son una receta biológica para incubar huevos. No se modifica el firmware al crear este manual.',M,748,CW,9,MUTED,maxh=39)

#2
begin('01 · La duda más frecuente','Tu caso: 32,6 °C y 55 %','¿Por qué no encienden calefacción y humidificador si todavía no llegaron al valor OFF?')
table(['Equipo','Encendido ON','Apagado OFF'],[['Calefacción','Por debajo de 32 °C','Al alcanzar 35,5 °C'],['Humidificador','Por debajo de 50 %','Al alcanzar 60 %']],188,[150,180,CW-330],50)
box('Si estaban APAGADOS','A 32,6 °C y 55 %, ambos continúan APAGADOS: todavía no cruzaron sus umbrales de encendido.',M,360,(CW-18)/2,148,GRAY)
box('Si estaban ENCENDIDOS','A esas mismas lecturas, ambos pueden seguir ENCENDIDOS: todavía no alcanzaron sus umbrales de apagado.',M+(CW+18)/2,360,(CW-18)/2,148,LIGHT)
note('<b>Una misma lectura admite dos estados correctos.</b> Para entender el resultado hay que conocer el estado anterior. Eso es la histéresis.',533,h=72)
y=633
y=bullet('La calefacción responde al frío','Enciende cuando la temperatura BAJA del umbral ON. No espera a que la temperatura suba.',y)
y=bullet('El humidificador responde a la humedad baja','Enciende por debajo de ON. Estar por debajo de OFF, por sí solo, no ordena encender.',y)
para('Supuesto de los ejemplos: parámetros aceptados, etapa correcta, control habilitado, sensor válido y sin una protección o prueba manual que tenga prioridad.',M,750,CW,9,MUTED,maxh=35)

#3
begin('02 · Mapa del sistema','Quién mide y quién decide','La ESP32 ejecuta el control aunque cierres la app o el teléfono pierda el WiFi.')
box('DHT22','Mide temperatura<br/>y humedad.',M,192,150,95)
box('DS3231','Mantiene fecha y hora<br/>con respaldo de batería.',W-M-150,192,150,95)
rect(201,326,193,104,INK)
text('ESP32',253,345,21,'Bold','#FFFFFF');para('Evalúa reglas, etapa<br/>y protecciones.',220,378,155,11,'#FFFFFF',maxh=40)
arrow(117,287,237,326);arrow(478,287,357,326)
box('App local','Configura y muestra.<br/>No sustituye al controlador.',M,462,150,104)
box('LCD 20 × 4','Título, temperatura,<br/>humedad y fecha/eventos.',W-M-150,462,150,104)
arrow(201,381,117,462);arrow(394,381,478,462)
arrow(297,430,297,590)
rect(M,591,CW,92,LIGHT,LINE)
for i,(name,gpio) in enumerate([('Motor','GPIO 25'),('Calefacción','GPIO 26'),('Ventilador','GPIO 27'),('Humidificador','GPIO 33')]):
 x=M+i*CW/4;text(name,x+10,611,10,'Bold');text(gpio,x+10,637,9,'Sans',MUTED)
para('<b>Ruta de una decisión:</b> lectura válida → regla de control → comprobación de seguridad → señal al relé → equipo físico. La app muestra la orden; sin sensor de retorno no puede demostrar que el equipo realmente giró, calentó o produjo humedad.',M,708,CW,11,maxh=67)

#4
begin('03 · Gráfica de calefacción','Enciende con frío; apaga al subir','Ejemplo didáctico: ON 32 °C / OFF 35,5 °C. Se parte con la calefacción apagada.')
chart([36,34,32.6,32,31.8,32.6,34,35.5,36],32,35.5,'heat','°C')
para('La franja verde es el intervalo donde se conserva el estado. t0, t1… son lecturas ilustrativas en orden; no representan minutos ni datos medidos de tu equipo.',M,525,CW,9.5,MUTED,maxh=42)
table(['Momento','Qué hace el control'],[['t3: exactamente 32 °C','Sigue OFF. El encendido exige bajar de 32.'],['t4: 31,8 °C','Pasa a ON: ya está por debajo del umbral.'],['t5: vuelve a 32,6 °C','Sigue ON: aún no llega a 35,5.'],['t7: 35,5 °C','Pasa a OFF.']],584,[160,CW-160],35,9.8)
para('La ventilación tiene prioridad. Una falla de sensor, sobretemperatura u otro bloqueo puede apagar la calefacción aunque esta gráfica indique demanda.',M,764,CW,8.5,MUTED,maxh=26)

#5
begin('04 · Gráfica de humedad','Enciende cuando falta humedad','Ejemplo didáctico: ON 50 % / OFF 60 %. Se parte con el humidificador apagado.')
chart([65,60,55,50,49,55,59,60,65],50,60,'humid','% HR')
para('A 55 % puedes ver ON u OFF, según la trayectoria. La lectura por sí sola no cuenta toda la historia.',M,527,CW,11,maxh=40)
box('Bajando desde una humedad alta','65 % → 55 %: sigue APAGADO.<br/>Solo arranca cuando baja de 50 %.',M,593,(CW-18)/2,115,GRAY)
box('Subiendo después de encender','49 % → 55 %: sigue ENCENDIDO.<br/>Se detiene al alcanzar 60 %. ',M+(CW+18)/2,593,(CW-18)/2,115,LIGHT)
note('<b>HR = humedad relativa.</b> 55 % es la lectura del ambiente. No significa que el tanque esté al 55 % ni indica cuánta agua queda.',730,h=53)

#6
begin('05 · Gráfica de ventilación','Enciende con calor; apaga al bajar','Ejemplo compatible con calefacción OFF 35,5 °C: ventilador ON 36 / OFF 35,5 °C.')
chart([34.8,35.5,36,36.2,35.8,35.5,34.8],36,35.5,'fan','°C')
para('A 36 °C exactos, un ventilador apagado sigue OFF. Arranca por encima de 36; permanece ON mientras la temperatura sea mayor que 35,5.',M,528,CW,11,maxh=46)
box('Al enfriarse','Cuando llega a 35,5 °C o menos, el ventilador se apaga. Esto no ordena encender automáticamente la calefacción.',M,596,CW,97)
note('<b>Los dos controles tienen reglas propias.</b> Si la calefacción ON está en 32 °C y venía apagada, esperará a que la temperatura baje de 32 °C. El ventilador térmico no reemplaza la renovación permanente de aire.',716,h=67)

#7
begin('06 · Guardar sin errores','Configura un conjunto coherente','El mensaje “revise la histéresis” puede venir de cualquiera de las dos etapas: incubación o nacimiento.')
rect(M,184,CW,68,LIGHT)
para('<b>Orden obligatorio de temperatura:</b><br/>Calefacción ON &lt; Calefacción OFF ≤ Ventilador OFF &lt; Ventilador ON',M+14,196,CW-28,11,maxh=48)
table(['Ejemplo de aprendizaje','Cal. ON / OFF','Vent. ON / OFF','HR ON / OFF'],[['A: tu último ejemplo','32 / 35,5','36 / 35,5','50 / 60'],['B: arranca a 32,6 °C y 55 %','33 / 33,5','34 / 33,5','56 / 60']],274,[169,112,119,CW-400],54,10)
text('Combinaciones que el sistema rechaza',M,438,13,'Bold')
table(['Configuración','Motivo'],[['Ventilador ON 33,5 / OFF 33,5','ON debe ser mayor que OFF.'],['Calefacción OFF 35,5 / ventilador OFF 33,5','Ventilador OFF no puede ser menor que calefacción OFF.'],['Humedad ON 60 / OFF 50','El encendido debe ser menor que el apagado.']],466,[245,CW-245],48,9.8)
para('<b>Otros límites:</b> calefacción ON ≥30 y OFF ≤39 °C; ventilador ON ≤40 °C; humedad ON ≥20 y OFF ≤85 %. Sobretemperatura debe superar el ON de ventilación de ambas etapas y ser ≤42 °C.',M,660,CW,10.5,maxh=65)
note('<b>Guardar no es escribir en el formulario.</b> Debe aparecer la confirmación del sistema. Con un ciclo activo o pausado no se permiten cambios de parámetros. No finalices un ciclo con huevos solo para ensayar estos ejemplos.',716,h=68)

#8
begin('07 · Etapas y botones','El ciclo tiene su propio reloj','El calendario depende del DS3231. El motor depende además de la etapa y las protecciones.')
# Línea temporal
x=M;y=220;w1=CW*18/21;w2=CW-w1
rect(x,y,w1,58,GREEN,r=0);rect(x+w1,y,w2,58,BLUE,r=0)
text('INCUBANDO · con volteo',x+14,y+18,12,'Bold','#FFFFFF');text('Nace',x+w1+10,y+18,11,'Bold','#FFFFFF')
for xx,lab in [(x,'0 h'),(x+w1-22,'432 h'),(x+CW-32,'504 h')]:text(lab,xx,y+67,10,'Bold')
para('0 a menos de 432 h: incubación. Desde 432 hasta menos de 504 h: nacimiento, sin motor. Al completar 504 h: finalizado. La opción B finaliza a las 432 h.',M,322,CW,11,maxh=53)
table(['Acción / estado','Efecto real'],[['Iniciar / nuevo ciclo','Exige reloj y sensor válidos e instalación confirmada. No reemplaza un ciclo existente.'],['Pausar','Congela el tiempo lógico y apaga el motor. El control ambiental continúa.'],['Reanudar','Continúa desde el tiempo acumulado; no vuelve al día 1.'],['Finalizar','Termina la automatización del ciclo. El ambiente sigue la política final configurada.'],['SIN_CICLO / FINALIZADO','Por defecto: salidas ambientales apagadas. Puede configurarse regulación fuera de ciclo.']],398,[150,CW-150],57,10)
para('<b>18 días cumplidos = 432 horas.</b> Eso corresponde al comienzo del día ordinal 19. “Día 18” en pantalla no significa necesariamente que ya se cumplieron las 432 horas.',M,737,CW,10,maxh=43)

#9
begin('08 · Uso de la app','Leer, configurar y confirmar','Conéctate al WiFi MINI_GRANJA_PAULETT y abre http://192.168.4.1. Usuario: admin; usa la clave configurada.')
# Panel ilustrado, claramente no captura real.
rect(M,191,CW,210,'#FFFFFF',LINE)
text('VISTA GENERAL',M+16,207,10,'Bold',GREEN);tag('INCUBANDO',W-M-123,200,107)
for i,(title,value) in enumerate([('TEMPERATURA','32,6 °C'),('HUMEDAD','55 %'),('DÍA DEL CICLO','3')]):
 xx=M+15+i*(CW-20)/3
 rect(xx,242,(CW-40)/3,76,LIGHT)
 text(title,xx+10,253,8,'Bold',MUTED);text(value,xx+10,275,22,'Bold')
text('Calefacción: OFF    Humidificador: OFF',M+18,337,11,'Bold')
text('Valores ilustrativos: no es una lectura del equipo.',M+18,370,9,'Sans',MUTED)
y=430
for n,title,body in [(1,'Revisa la vista general','Comprueba etapa, reloj, sensor, alarmas y estado de cada equipo. “Automático” es un modo, no significa encendido.'),(2,'Configura antes de iniciar','Revisa ambas etapas, motor y políticas. En un ciclo activo o pausado, el guardado está bloqueado.'),(3,'Guarda y espera confirmación','Si aparece un error, los valores nuevos no quedaron aplicados. Recargar la página muestra los valores guardados.'),(4,'Inicia cuando el equipo esté listo','Ajusta la fecha inicial y confirma. Cerrar el navegador no detiene el ciclo ni los temporizadores.')]:y=bullet(title,body,y,n)
para('Pantalla LCD: fila 1 título; fila 2 temperatura; fila 3 humedad; fila 4 fecha y hora, sustituida temporalmente por eventos. Guiones en mediciones indican datos no confiables.',M,738,CW,10,maxh=45)

#10
begin('09 · Motor de volteo','Duración e intervalo no son lo mismo','La duración es cuánto tiempo gira. El intervalo es cuánto espera entre movimientos programados.')
rect(M,192,CW,143,'#FFFFFF',LINE)
text('Ejemplo: duración 10 s / intervalo 3 h',M+16,207,12,'Bold')
line(M+24,284,W-M-22,284,MUTED,2)
for xx,label in [(M+30,'Movimiento 1'),(W-M-60,'Movimiento 2')]:
 rect(xx,249,22,35,GREEN,r=2);text('10 s',xx-1,230,9,'Bold',GREEN);text(label,xx-15,300,9)
arrow(M+69,267,W-M-75,267,BLUE);text('3 horas entre inicios',211,244,11,'Bold',BLUE)
text('Esquema no proporcional: el pulso se amplía para verlo.',M+16,347,9,'Sans',MUTED)
table(['Para que funcione','Debe cumplirse'],[['Etapa','INCUBANDO y menos de 432 horas acumuladas.'],['Calidad y seguridad','RTC y sensor válidos; sin sobretemperatura; NVS e instalación correctas.'],['Duración','1 s hasta el máximo configurado. Máximo absoluto: 120 s.'],['Intervalo automático','10 a 1.440 minutos; valor inicial: 180 minutos.'],['Prueba manual','Usa la duración configurada. No funciona en SIN_CICLO, PAUSADO, NACIMIENTO ni FINALIZADO.']],383,[157,CW-157],50,10)
box('Qué pasa si se corta la energía','Al volver, el motor queda apagado y se omiten los movimientos vencidos. Un intento incierto no se repite. La prueba manual reprograma el siguiente movimiento para un intervalo completo después.',M,688,CW,95)

#11
begin('10 · Salidas y relés','HIGH enciende; LOW apaga','Esta es la polaridad configurada para los cuatro canales desde la actualización del 02/11/2026.')
table(['Canal','GPIO','Encendido','Apagado'],[['1 · Motor','25','HIGH','LOW'],['2 · Calefacción','26','HIGH','LOW'],['3 · Ventilador','27','HIGH','LOW'],['4 · Humidificador','33','HIGH','LOW']],186,[199,72,120,CW-391],37,10.5)
text('Selector de activación: ejemplo conceptual',M,406,13,'Bold')
rect(M,439,CW,106,'#FFFFFF',LINE)
for xx,lab in [(M+48,'LOW'),(M+140,'COM'),(M+232,'HIGH')]:
 text(lab,xx-10,457,10,'Bold');c.setFillColor(HexColor(INK));c.circle(xx+3,H-502,5,fill=1,stroke=0)
line(M+143,502,M+235,502,GREEN,9)
para('Puente COM-HIGH<br/><b>solo si así lo identifica<br/>el selector de tu módulo.</b>',M+300,461,190,10,maxh=63)
para('El selector HIGH/COM/LOW no es lo mismo que las borneras de potencia COM/NO/NC. No cambies jumpers con la alimentación conectada. El dibujo no representa un modelo específico de placa.',M,565,CW,10.5,maxh=57)
box('Lo que indica la app y lo que ocurre físicamente','App ON + equipo sin funcionar: revisar canal, jumper, alimentación y conexiones con las cargas desenergizadas. App OFF: revisar demanda, etapa y protecciones antes de atribuirlo al cableado.',M,642,CW,103)
para('El firmware no reemplaza fusibles, aislamiento ni un termostato independiente de seguridad. En reset hay alta impedancia: el reposo debe quedar garantizado por la electrónica externa.',M,756,CW,8.5,MUTED,maxh=27)

#12
begin('11 · Diagnóstico','Primero mira la orden en la app','No fuerces las salidas para ocultar una alarma. Identifica qué condición está impidiendo el funcionamiento.')
table(['Síntoma','Qué significa / qué revisar'],[['Todo OFF con 32,6 °C y 55 %','Puede ser normal con ON 32 / 50 y estado previo apagado. Revisa la histéresis.'],['ERROR SENSOR','Lectura inválida o antigua. Calefacción, humedad y motor se bloquean; ventilación según política.'],['ERROR RTC','Puede faltar hora válida, comunicación o avance del reloj. El motor se bloquea y la marcha del ciclo se pausa.'],['Sobretemperatura','Apaga calefacción, humedad y motor; demanda ventilación. Prima sobre el control normal.'],['Instalación pendiente','Salidas bloqueadas hasta confirmar la revisión eléctrica en configuración.'],['ERROR_CRITICO / NVS','Estado no confiable o fallo de almacenamiento/temporizador. Requiere mantenimiento.'],['No guarda parámetros','Ciclo activo/pausado, prueba en curso o valores incompatibles. No basta editar el formulario.']],184,[161,CW-161],59,10)
box('Si el RTC fue conectado por primera vez','En “Reloj DS3231”: usar hora del teléfono → ajustar reloj → confirmar. Pausa antes si hay un ciclo en marcha. Si el error persiste, revisar diagnóstico I2C y reloj; no asumir que es solo batería.',M,660,CW,101)
para('Lecturas cada 2,5 s; encendidos sujetos a 5 s desde el último cambio. La app redondea a una decimal: el valor interno puede cruzar un umbral antes de que notes el cambio visual.',M,764,CW,8.3,MUTED,maxh=24)

#13
begin('12 · Glosario A','Palabras de medición y control','Definiciones cortas para interpretar gráficas, formularios y comportamiento del equipo.')
glossary([
('MVP','Producto mínimo viable: versión funcional inicial del sistema. No significa que esté certificado para cualquier instalación.'),
('Histéresis','Dos umbrales separados y memoria del estado anterior. Evita cambios rápidos cerca de una misma lectura.'),
('Umbral','Valor que dispara una decisión. Por ejemplo: encender calefacción cuando la temperatura baja de 32 °C.'),
('ON / OFF','Encendido / apagado. En formularios son umbrales; en tarjetas de equipos son estados actuales.'),
('Estado anterior','Cómo estaba el relé antes de la lectura actual. Importa cuando el valor está entre los dos umbrales.'),
('Banda de histéresis','Intervalo entre ON y OFF. Dentro de él se conserva el estado, salvo que intervenga una protección.'),
('Temperatura / °C','Medición térmica del sensor, expresada en grados Celsius. La calefacción responde a temperatura baja.'),
('Humedad relativa / HR','Porcentaje de humedad relativa del aire. No indica el nivel de agua del humidificador.'),
('Consigna','Objetivo o referencia de control. En este MVP se usan umbrales ON/OFF; no hay una única casilla de consigna.'),
('Lectura válida','Medición aceptada por el sistema. Si falla la lectura o es demasiado antigua, no se usa para regular.'),
('Muestreo','Frecuencia con la que se lee el sensor. Aquí se consulta el DHT22 cada 2,5 segundos.'),
('Sobretemperatura','Límite de emergencia independiente de la histéresis normal. Da prioridad a la ventilación y bloquea calor, humedad y motor.')
])

#14
begin('13 · Glosario B','Palabras del equipo y del ciclo','El modo de un actuador, el estado del ciclo y el nivel eléctrico son conceptos distintos.')
glossary([
('ESP32 / firmware','La placa controladora y el programa grabado en ella. Ejecutan las reglas sin depender del teléfono.'),
('DHT22 / DS3231','DHT22: sensor de temperatura y humedad. DS3231: reloj de fecha y hora, también llamado RTC.'),
('Relé / GPIO','Relé: interruptor controlado eléctricamente. GPIO: pin de señal de la ESP32; no alimenta directamente la carga.'),
('HIGH / LOW','Nivel alto / bajo de una señal. En esta instalación, HIGH ordena encender y LOW ordena apagar.'),
('AP / SSID / IP','AP: punto de acceso WiFi. SSID: nombre de la red. IP: dirección del panel, 192.168.4.1.'),
('I2C / LCD','I2C: bus compartido por reloj y pantalla. LCD: pantalla local; la instalada tiene 20 columnas y 4 filas.'),
('AUTOMATICO','El actuador sigue las reglas. Puede estar encendido o apagado: automático no significa ON permanente.'),
('MANUAL_TEMPORAL','Prueba por tiempo limitado. Respeta protecciones y luego devuelve el actuador al control automático.'),
('DESHABILITADO / BLOQUEADO','Deshabilitado: no corresponde actuar. Bloqueado por seguridad: falta una condición confiable o existe una alarma.'),
('NVS / LittleFS','NVS guarda configuración y ciclo. LittleFS guarda archivos de la interfaz web. No son la misma memoria lógica.'),
('Pausa / tiempo lógico','Pausar congela la edad del ciclo. El tiempo lógico acumula solo la marcha; el reloj real sigue avanzando.'),
('Watchdog / evento','Watchdog: supervisa el control y puede reiniciar ante bloqueo. Evento: algo que ocurrió, no siempre una falla. Se guardan 48 eventos en RAM; se pierden al reiniciar.')
])

#15
begin('14 · Hoja rápida','Consulta antes de cambiar algo','Esta página resume las reglas de control normal. Las protecciones siempre tienen prioridad.')
table(['Equipo','Arranca desde OFF','Se detiene desde ON'],[['Calefacción','T < ON','T ≥ OFF'],['Humidificador','HR < ON','HR ≥ OFF'],['Ventilador','T > ON','T ≤ OFF']],185,[151,180,CW-331],39,11)
box('Antes de guardar','1. Revisar ambas etapas.<br/>2. Cal. ON &lt; Cal. OFF ≤ Vent. OFF &lt; Vent. ON.<br/>3. HR ON &lt; HR OFF.<br/>4. Confirmar el guardado; no editar durante un ciclo.',M,366,CW,121)
text('Valores iniciales del firmware, no lectura de tu NVS',M,512,12,'Bold')
table(['Etapa','Cal. ON / OFF','Vent. ON / OFF','HR ON / OFF'],[['Incubación','37,5 / 37,8','38 / 37,8','50 / 60'],['Nacimiento','37,2 / 37,5','37,8 / 37,5','65 / 70']],542,[135,121,131,CW-387],36,10)
para('<b>No confundir:</b> los valores de esta tabla son los iniciales del programa. Tu equipo puede tener otros guardados. No se ha leído ni modificado su configuración para elaborar este documento.',M,662,CW,9.5,maxh=43)
para('<b>Base técnica del manual:</b> src/config.h (reglas y límites), src/app.cpp (ciclo, prioridades y validación), src/relays.cpp (GPIO y temporizadores), src/rtc_manager.cpp, src/lcd_manager.cpp, src/storage.cpp y data/app.js. Revisión del proyecto: 02/11/2026. Diagramas y gráficas son ilustraciones originales, no mediciones reales.',M,716,CW,9,MUTED,maxh=67)

assert page==15
c.save()
print(OUT)

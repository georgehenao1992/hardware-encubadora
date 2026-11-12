'use strict';
const $ = id => document.getElementById(id);
const relayNames = ['Motor de volteo','Calefacción','Ventilación','Humidificador'];
const keys = ['heatOn','heatOff','fanOn','fanOff','humidOn','humidOff'];
const labels = ['Calefacción ON (°C)','Calefacción OFF (°C)','Ventilador ON (°C)','Ventilador OFF (°C)','Humedad ON (%)','Humedad OFF (%)'];
let auth='', busy=false, lastStatus=null, fetching=false;
for(const [stage,title] of [['incubation','Incubación · primeras 432 horas'],['birth','Nacimiento · últimas 72 horas']]) {
 const card=document.createElement('div');card.className='stage';
 const titleEl=document.createElement('h3');titleEl.textContent=title;card.append(titleEl);
 const grid=document.createElement('div');grid.className='form-grid';
 keys.forEach((key,i)=>{const label=document.createElement('label');label.textContent=labels[i];const input=document.createElement('input');Object.assign(input,{name:`${stage}_${key}`,type:'number',step:'0.1',min:i<4?'30':'20',max:i<4?'40':'85',required:true});label.append(input);grid.append(label);});
 card.append(grid);$('stages').append(card);
}
relayNames.forEach((name,i)=>{
 const label=document.createElement('label');label.textContent=name;const select=document.createElement('select');select.name=`low${i}`;
 for(const [value,text] of [['true','Activo LOW · reposo HIGH'],['false','Activo HIGH · reposo LOW']]) {const option=document.createElement('option');option.value=value;option.textContent=text;select.append(option);}label.append(select);$('polarities').append(label);
 const card=document.createElement('article');card.className='equipment';card.id=`relay${i}`;
 const status=document.createElement('div');status.className='relay-state';const word=document.createElement('span');word.textContent='APAGADO';status.append(word,document.createElement('i'));
 const heading=document.createElement('h3');heading.textContent=name;const mode=document.createElement('p');mode.className='mode';
 const button=document.createElement('button');button.textContent=i===0?'Probar motor':'Probar 10 segundos';button.type='button';button.onclick=()=>command({action:'test',relay:i,seconds:10},`¿Probar ${name.toLowerCase()}? Se respetan los límites de seguridad.`);
 card.append(status,heading,mode,button);$('relays').append(card);
});
function notice(text,error=false) {$('notice').textContent=text;$('notice').className=error?'error':'success';$('notice').hidden=false;}
function epoch(value) {if(!value)return 0;return Math.floor(Date.parse(value+'Z')/1000);}
function dateInput(t) {return new Date(t*1000).toISOString().slice(0,19);}
function formatted(t) {return t?new Intl.DateTimeFormat('es-CO',{dateStyle:'short',timeStyle:'medium',hourCycle:'h23',timeZone:'UTC'}).format(new Date(t*1000)):'—';}
function localNow() {const d=new Date();return new Date(Date.UTC(d.getFullYear(),d.getMonth(),d.getDate(),d.getHours(),d.getMinutes(),d.getSeconds())).toISOString().slice(0,19);}
async function api(path,body) {
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),6000);
 try {const response=await fetch(path,{method:body?'POST':'GET',headers:{Authorization:`Basic ${auth}`,...(body?{'Content-Type':'application/json','X-Paulett-Request':'1'}:{})},body:body?JSON.stringify(body):undefined,signal:controller.signal,cache:'no-store'});
 const data=await response.json();if(!response.ok)throw new Error(data.error||`Error ${response.status}`);return data;
 }finally{clearTimeout(timeout);}
}
function render(s) {
 lastStatus=s;$('connection').textContent='Conectado · local';$('connection').className='badge online';$('dashboard').classList.remove('stale');
 $('temperature').textContent=s.sensorOK&&Number.isFinite(s.temperature)?s.temperature.toFixed(1):'—';$('humidity').textContent=s.sensorOK&&Number.isFinite(s.humidity)?s.humidity.toFixed(1):'—';
 $('tempHint').textContent=s.sensorOK?'DHT22 · lectura confiable':'ERROR SENSOR';$('humidHint').textContent=s.sensorAge>=0?`Última lectura válida hace ${(s.sensorAge/1000).toFixed(0)} s`:'Sin lectura válida';
 $('rtc').textContent=s.rtcOK?formatted(s.now):'ERROR RTC · revisar reloj';$('state').textContent=s.state.replaceAll('_',' ');$('day').textContent=s.day||'—';
 const total=s.elapsed+s.remaining;$('totalDays').textContent=`/ ${total===1555200?'18':'21'} días`;$('progress').max=total||1814400;$('progress').value=s.elapsed;
 const days=Math.floor(s.remaining/86400),hours=Math.floor(s.remaining%86400/3600);$('remaining').textContent=s.state==='SIN_CICLO'?'Listo para un nuevo ciclo':s.state==='FINALIZADO'?'Ciclo finalizado':`${days} días y ${hours} horas restantes${s.state==='PAUSADO'?' · en pausa':''}`;
 $('nextTurn').textContent=formatted(s.nextTurn);$('lastAttempt').textContent=formatted(s.lastAttempt);$('lastConfirmed').textContent=formatted(s.lastConfirmed);
 $('simulation').hidden=!s.simulation;$('simControls').hidden=!s.simulation;
 $('alarms').replaceChildren(...s.alarms.map(text=>{const el=document.createElement('div');el.className='alarm';el.textContent=text;return el;}));
 s.relays.forEach((r,i)=>{const card=$(`relay${i}`);card.classList.toggle('on',r.on);card.querySelector('.relay-state span').textContent=r.on?'ENCENDIDO':'APAGADO';card.querySelector('.mode').textContent=r.mode.replaceAll('_',' ');card.querySelector('button').disabled=busy||!s.hardwareConfirmed||!s.sensorOK||!s.storageOK||(i===0&&(s.state!=='INCUBANDO'||!s.rtcOK));});
 $('events').replaceChildren(...s.events.map(event=>{const row=document.createElement('tr');for(const text of [formatted(event.time),event.code,event.description]){const td=document.createElement('td');td.textContent=text;row.append(td);}return row;}));
 const active=['INCUBANDO','NACIMIENTO','PAUSADO'].includes(s.state);
 document.querySelectorAll('[data-action]').forEach(button=>{const a=button.dataset.action;button.disabled=busy||!s.storageOK||(a==='pause'?!['INCUBANDO','NACIMIENTO'].includes(s.state):a==='resume'?s.state!=='PAUSADO'||!s.rtcOK:a==='finish'?!active:active||!s.rtcOK||!s.sensorOK||!s.hardwareConfirmed);});
 $('settings').querySelector('button[type=submit]').disabled=active||busy;
 if(!$('startDate').value&&s.now)$('startDate').value=dateInput(s.now);
 if(!$('clockDate').value&&s.now)$('clockDate').value=dateInput(s.now);
}
async function refresh() {
 if(!auth||fetching)return;fetching=true;
 try {render(await api('/api/status'));}catch(error){$('connection').textContent='Sin conexión · datos antiguos';$('connection').className='badge';$('dashboard').classList.add('stale');document.querySelectorAll('[data-action],.equipment button').forEach(b=>b.disabled=true);notice(`No se pudo actualizar: ${error.message}. El control local continúa.`,true);}finally{fetching=false;}
}
function fillSettings(c) {
 const f=$('settings').elements;
 for(const stage of ['incubation','birth']) for(const key of keys)f[`${stage}_${key}`].value=c[stage][key];
 for(const key of ['motorSeconds','motorMaxSeconds','overtemp','idlePolicy','lcdAddress'])f[key].value=c[key];
 for(const key of ['finishAt18','emergencyFan'])f[key].value=String(c[key]);
 for(const key of ['automaticTurn','hardwareConfirmed'])f[key].checked=c[key];
 f.intervalMinutes.value=c.intervalSeconds/60;f.firstMovement.value=c.firstMovement?dateInput(c.firstMovement):'';
 c.activeLow.forEach((value,i)=>f[`low${i}`].value=String(value));
}
async function command(body,message='¿Confirmas este cambio?') {
 if(busy||!window.confirm(message))return false;busy=true;if(lastStatus)render(lastStatus);
 try {await api('/api/command',{...body,confirm:true});notice('Cambio aplicado en la incubadora.');await refresh();return true;}catch(error){notice(error.message,true);return false;}finally{busy=false;if(lastStatus)render(lastStatus);}
}
$('login').onsubmit=async event=>{
 event.preventDefault();const button=$('login').querySelector('button');button.disabled=true;
 try {auth=btoa(`admin:${$('password').value}`);const s=await api('/api/status');fillSettings(await api('/api/config'));$('login').hidden=true;$('dashboard').hidden=false;$('logout').hidden=false;$('password').value='';$('notice').hidden=true;render(s);}catch(error){auth='';notice(error.message,true);}finally{button.disabled=false;}
};
$('logout').onclick=()=>{auth='';lastStatus=null;$('dashboard').hidden=true;$('login').hidden=false;$('logout').hidden=true;$('notice').hidden=true;$('connection').textContent='Sesión cerrada';$('connection').className='badge';};
const actionMessages={start:'¿Iniciar el ciclo con la fecha indicada?',new:'¿Crear un nuevo ciclo? Se reemplazará el registro del ciclo finalizado.',pause:'¿Pausar el ciclo? El motor se detendrá y el control ambiental continuará.',resume:'¿Reanudar el ciclo? Verifica primero el reloj y el tiempo acumulado.',finish:'¿Finalizar el ciclo? Se aplicará la política ambiental configurada.'};
document.querySelectorAll('[data-action]').forEach(button=>button.onclick=()=>{
 const action=button.dataset.action;if(['start','new'].includes(action)&&!$('startDate').value){notice('Selecciona fecha y hora de inicio.',true);return;}
 command({action,start:epoch($('startDate').value)},actionMessages[action]);
});
$('settings').onsubmit=async event=>{
 event.preventDefault();const f=$('settings').elements;const c={action:'config'};
 for(const stage of ['incubation','birth']){c[stage]={};for(const key of keys)c[stage][key]=Number(f[`${stage}_${key}`].value);const s=c[stage];if(!(s.heatOn<s.heatOff&&s.fanOff>=s.heatOff&&s.fanOn>s.fanOff&&s.humidOn<s.humidOff)){notice('Revisa la histéresis y los límites de ambas etapas.',true);return;}}
 for(const key of ['motorSeconds','motorMaxSeconds','overtemp','idlePolicy','lcdAddress'])c[key]=Number(f[key].value);
 for(const key of ['finishAt18','emergencyFan'])c[key]=f[key].value==='true';
 for(const key of ['automaticTurn','hardwareConfirmed'])c[key]=f[key].checked;
 c.intervalSeconds=Number(f.intervalMinutes.value)*60;c.firstMovement=epoch(f.firstMovement.value);c.activeLow=[0,1,2,3].map(i=>f[`low${i}`].value==='true');
 if(c.motorSeconds>c.motorMaxSeconds||c.overtemp<=Math.max(c.incubation.fanOn,c.birth.fanOn)){notice('Duración del motor o umbral de sobretemperatura incompatible.',true);return;}
 if(await command(c,'¿Guardar parámetros? Si cambiaste polaridad, confirma que las cargas están desconectadas y que verificaste el reposo eléctrico.'))fillSettings(await api('/api/config'));
};
$('phoneTime').onclick=()=>{$('clockDate').value=localNow();};
$('clockForm').onsubmit=event=>{event.preventDefault();command({action:'clock',epoch:epoch($('clockDate').value)},'¿Ajustar el DS3231 a la fecha y hora local indicada?');};
$('credentials').onsubmit=async event=>{event.preventDefault();const f=$('credentials').elements;const wifi=f.wifi.value,admin=f.admin.value;
 if(busy||!window.confirm('¿Cambiar ambas claves? Guarda las nuevas credenciales antes de continuar. WiFi cambiará después de reiniciar.'))return;
 busy=true;try{await api('/api/command',{action:'credentials',wifi,admin,confirm:true});auth=btoa(`admin:${admin}`);$('credentials').reset();notice('Claves guardadas. La clave WiFi se aplica al reiniciar.');await refresh();}catch(error){notice(error.message,true);}finally{busy=false;}
};
$('stopTests').onclick=()=>command({action:'stopTests'},'¿Terminar las pruebas y devolver las salidas al control automático?');
document.querySelectorAll('[data-elapsed]').forEach(button=>button.onclick=()=>command({action:'simulate',elapsed:Number(button.dataset.elapsed)},'¿Saltar a este instante del ciclo simulado?'));
setInterval(refresh,2500);

# Protocolo de validación

Las pruebas físicas siguientes están **pendientes de ejecutar en la incubadora**. No se ha medido el cableado, el movimiento mecánico ni la temperatura real. Guardar para cada prueba fecha, resultado, instrumento y responsable.

## Pruebas automáticas locales

`./scripts/test.sh` compila y ejecuta la lógica C++ con advertencias como errores, después revisa sintaxis JS. Cubre exactamente 432/504 horas, opción A/B, pausa/reanudación, prevención de subdesbordamiento por reloj atrasado, ranuras futuras después de cortes, histéresis y exclusión de calefacción/ventilación en miles de combinaciones, límites y NaN. No sustituye ensayos de NVS real, interrupción eléctrica, servidor HTTP ni hardware.

Compilar además `pio run -e esp32dev -e simulation` y `pio run -e esp32dev -t buildfs`. La biblioteca LCD puede emitir advertencias por macros binarias antiguas; no son errores del firmware.

## Relés

1. Con cargas desconectadas y hardware no confirmado: medir entradas durante energización, reset y firmware. No debe activarse ningún contacto. Si ocurre, corregir driver/pull-ups; no compensar solo en código.
2. Configurar cada polaridad y confirmar con potencia desconectada. Comprobar reposo LOW/HIGH real. Cambiar y verificar cada canal por separado.
3. Motor: con RTC y sensor válidos, iniciar ciclo. Usar «Probar motor». Medir duración y apagado. Cerrar navegador y apagar WiFi del celular durante la prueba: debe apagarse igualmente.
4. Intentar motor en SIN_CICLO, PAUSADO, NACIMIENTO, FINALIZADO y con error RTC/sensor. Debe rechazarse. El límite absoluto 120 s no debe aceptar 121 ni cero ni negativos ni tipos string.
5. Calefacción: simular temperatura baja por método de banco seguro. Debe encender por debajo de heatOn y apagar al alcanzar heatOff. Una prueba manual con T ≥ heatOff no debe encenderla.
6. Ventilador: T > fanOn enciende; al descender a fanOff apaga. Solicitar prueba mientras calefacción encendida: apagar calefacción antes de ventilar. Nunca ambos.
7. Humidificador: HR < humidOn enciende, HR ≥ humidOff apaga. Probar límites por etapa.
8. Verificar separación mínima de 5 s para encendidos y apagado inmediato de seguridad. Las pruebas ambientales desde panel duran 10 s y API permite 1–30 s. No se prolongan por pulsaciones repetidas; al expirar la regulación automática puede mantener una salida si la necesita.
9. Verificar política sin ciclo OFF y REGULATE; finalizar manualmente y por tiempo para cada una. Las alarmas de sobretemperatura y sensor conservan su prioridad aun fuera de ciclo.

## Sensores y reloj

| Ensayo | Resultado esperado |
|---|---|
| DHT desconectado al arrancar | ERROR SENSOR, calefacción/humedad OFF; evento tras primera lectura |
| Desconectar DHT mientras calienta | Corte con primera lectura fallida, dentro del siguiente muestreo |
| Valores NaN, T fuera 0–60, HR fuera 1–100 | Rechazo; conservar último válido solo para diagnóstico |
| Lectura con más de 7 s | Bloquear regulación basada en ella |
| Sensor recuperado | SENSOR OK y regulación según política; motor espera nueva ranura |
| T por encima de emergencia | Ventilador ON, motor/calefacción/humedad OFF |
| Valor plausible pero fuera del objetivo | Alarma ambiental, distinta de ERROR SENSOR |
| DS3231 ausente / batería perdida | Reloj bloqueado; no iniciar ni reanudar |
| RTC retrocede / salta más de 5 s en marcha | Pausa segura y motor OFF |
| RTC detenido más de 5 s | Pausa segura |
| Ajustar hora en marcha | Rechazo, requiere pausa |
| LCD ausente | Evento; control continúa |
| LCD 0x27 y 0x3F / filas 3 y 4 | Verificar dirección y alineación en 20 columnas |
| LCD normal | Filas fijas: titulo, temperatura y humedad; fecha/hora 24 h en cuarta fila, sustituida temporalmente por eventos |

Para inyectar valores sin equipo, usar pruebas de lógica. No habilitar fuentes de sensor ficticias en producción. Un fallo intermitente del bus I2C debe probarse físicamente: el diagnóstico de presencia y las lecturas no sustituyen un bus eléctricamente confiable.

## Simulación acelerada

```sh
pio run -e simulation
pio run -e simulation -t upload
pio run -e simulation -t uploadfs
```

Solo este firmware incluye `action:simulate`. Todos los accesos de salida física a relés se eliminan por compilación; se mantiene únicamente el estado lógico. NVS usa `paulett-test`, separado de `paulett`; ambos espacios reciben inicialmente `CAMBIA_ESTA_CLAVE` y conservan sus cambios posteriores de forma independiente. RTC y DHT siguen siendo los periféricos reales para poder ensayar su falla.

1. Conectar sensores/reloj y entrar con las claves de simulación. Verificar banda MODO DE SIMULACIÓN.
2. Confirmar instalación lógica, configurar modo A e iniciar.
3. «Día 1»: elapsed=0, INCUBANDO, motor lógicamente permitido.
4. «Antes de 432 h»: elapsed=1.555.199; tras un segundo pasa a NACIMIENTO, motor OFF.
5. «432 h»: elapsed=1.555.200, nacimiento y umbrales de nacimiento.
6. «504 h»: elapsed=1.814.400, FINALIZADO y política final aplicada.
7. Finalizar si hace falta, elegir opción B, iniciar otro ciclo, saltar a 432 h: FINALIZADO sin nacimiento.
8. Probar pausas y cortes. No es un reloj que corra a velocidad alterada: los botones sitúan el ciclo en instantes de prueba, después sigue a velocidad real.
9. Recargar **esp32dev** para operar. Mantener cargas desconectadas mientras verifica la recuperación del espacio NVS real.

## Recuperación

| Momento del corte | Resultado al volver |
|---|---|
| Sin motor activo, RTC correcto | Reconstruir edad; omitir todas las ranuras vencidas |
| Durante pulso de motor | GPIO apagado; MOTOR INCIERTO; no repetir intento |
| Entre marca NVS y pulso | Consumir intento aunque no haya girado |
| Tras apagado físico, antes de guardar confirmación | Considerar incierto; no duplicar |
| Apagón atravesando varias ranuras | Siguiente ranura futura; ningún encendido inmediato por atrasos |
| Corte exactamente en ranura | Omitir esa ranura al arrancar |
| Corte durante pausa | Seguir pausado y mantener tiempo lógico |
| Apagón atraviesa 432 h | Nacimiento o fin según opción, motor OFF |
| Apagón atraviesa 504 h | Finalizado; política ambiental aplicada |
| RTC inválido al volver | Pausa con último tiempo persistido; operador reconstruye tiempo perdido |
| Fallo de escritura / registro corrupto | ERROR_CRITICO y salidas bloqueadas |
| Reinicio con LittleFS no cargado | Control autónomo; HTTP informa que debe cargar archivos |

Repetir con distintos largos de corte y observar contactos con instrumento independiente. Documentar `lastAttempt`, `lastConfirmed`, próximo movimiento y eventos antes/después. No asumir que el texto «completado» demuestra movimiento mecánico.

## Panel y acceso

Probar móvil y escritorio; 401 sin clave, 429 tras diez fallos por minuto, rechazo de POST sin cabecera propia, confirmación obligatoria, límites negativos/NaN/campos ausentes y ajustes bloqueados durante ciclo. Cambiar claves y probar nueva autenticación; WiFi cambia después de reinicio. La fecha debe conservar la hora local incluso si el navegador usa otra zona. Verificar que no se envían recursos ni datos a Internet.

## Lista previa a operar

- [ ] Placa, GPIO y flash verificados; cableado registrado.
- [ ] Pull-ups, drivers, relés y cargas con niveles y polaridad medidos.
- [ ] Sin activación en reset, bootloader o desconexión de ESP32.
- [ ] I2C sin 5 V en GPIO; DHT con alimentación y pull-up correctos.
- [ ] Fusibles, aislamiento, puesta a tierra y protección de cargas instalados.
- [ ] Termostato físico independiente probado con calefacción real.
- [ ] Motor recorrido y atasco evaluados; duración validada; finales de carrera si corresponden.
- [ ] RTC y batería revisados, fecha local correcta y comportamiento ante pérdida de batería ensayado.
- [ ] DHT cotejado con termómetro/higrómetro de referencia; ubicación representativa.
- [ ] Renovación permanente de aire además del ventilador térmico.
- [ ] Políticas pausa, final y emergencia conocidas por el operador.
- [ ] Todas las pruebas de cortes completadas sin movimientos duplicados.
- [ ] Pruebas sostenidas en vacío de temperatura/humedad y uniformidad espacial satisfactorias.
- [ ] Claves guardadas; USB físicamente protegido.
- [ ] Firmware de producción cargado; simulación deshabilitada.
- [ ] Supervisión y método de contingencia ante falla establecidos.

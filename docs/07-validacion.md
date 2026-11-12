# Resultado de validación de software

Revisión realizada el 29 de octubre de 2026, sin conectar ni programar una ESP32 física.

| Verificación ejecutada | Resultado |
|---|---|
| Compilación PlatformIO `esp32dev` | Correcta |
| Compilación PlatformIO `simulation` | Correcta |
| Generación de imagen LittleFS | Correcta; incluye HTML, CSS y JavaScript |
| Pruebas nativas C++ de lógica | Correctas |
| Sintaxis JavaScript con `node --check` | Correcta |
| Sintaxis de scripts Python | Correcta |
| Enlaces locales de documentación | Correctos |
| Exportación de sketch Arduino | Generada desde las fuentes finales |
| Arduino IDE directamente | No ejecutada; compilación comprobada con framework Arduino en PlatformIO |
| Revisión visual en navegador | No ejecutada: navegador conectado no disponible y servicio de control gráfico no inició |
| Pruebas eléctricas y de periféricos reales | Pendientes; seguir `05-pruebas.md` |

Compilación de producción anterior al cambio de credenciales: **55.800 bytes de RAM estática (17,0%)** y **1.225.572 bytes de aplicación (93,5%)**. Simulación: 55.800 bytes de RAM y 1.226.104 bytes de aplicación. La RAM indicada no incluye asignaciones dinámicas en ejecución; verificar estabilidad en banco y no agregar bibliotecas pesadas sin revisar el espacio de aplicación.

Las pruebas nativas cubren los límites temporales, histéresis y exclusión de calefacción/ventilación; no ejecutan toda la máquina de estados con periféricos emulados. Las pruebas de cortes y corrupción de NVS descritas son un protocolo pendiente de banco, no resultados experimentales obtenidos.

Se corrigió explícitamente el direccionamiento DDRAM de las filas de LCD 16×4 a 0x00, 0x40, 0x10 y 0x50; la biblioteca fijada usa offsets de 20×4 en su método setCursor. Debe comprobarse el mapeo real del backpack.

## Vista local de diseño

```sh
python3 scripts/preview.py
```

Abrir `http://127.0.0.1:8080`; se acepta cualquier texto de al menos 8 caracteres en el formulario de acceso. Es exclusivamente una **vista con datos ficticios**, enlazada solo a localhost, con avisos visibles y todas las mutaciones rechazadas. No tiene conexión con la incubadora y no sustituye una prueba del servidor ESP32. Detener con Ctrl+C. En entornos restringidos puede necesitar permiso para abrir el puerto local.

## Actualización de credenciales

Se configuró `CAMBIA_ESTA_CLAVE` para WiFi y administrador, con migración única de NVS y mínimo administrativo de 8 caracteres en firmware e interfaz. Las pruebas nativas y la comprobación de JavaScript pasaron nuevamente. Esta modificación requiere recargar firmware y LittleFS; no se ha cargado automáticamente al equipo.

## Ajuste del LCD solicitado el 30 de octubre de 2026

Las filas 1–3 permanecen dedicadas a GRANJA PAULETT, temperatura y humedad. Sin una lectura confiable se muestran guiones conservando las etiquetas. La cuarta fila muestra hora y fecha; cada evento la sustituye durante 4,5 segundos. Las alarmas persistentes y el fin de ciclo alternan con la fecha cada cinco segundos. Se retiró la rotación rutinaria de etapa y próximo volteo del LCD; esos datos siguen disponibles en el panel web. Se conserva el direccionamiento 16×4 y la actualización sin limpieza continua.

## Corrección según fotografía del equipo

La pantalla instalada tiene 20 columnas y 4 filas, a diferencia de las 16 columnas indicadas inicialmente. La fotografía muestra HUM en las últimas cuatro columnas de la primera fila y ERROR RTC partido entre segunda y cuarta: corresponde a escribir las filas 3/4 en 0x10/0x50 sobre un módulo 20×4. Se corrigen a 0x14/0x54, se inicializa el LCD como 20×4 y se rellenan las 20 posiciones de cada fila para evitar residuos. Este ajuste sustituye el direccionamiento 16×4 documentado en las revisiones anteriores. ERROR RTC es un aviso independiente del diseño: la fecha requiere que el DS3231 tenga una hora válida.

## Diagnóstico físico del DS3231 — 30/10/2026

El módulo respondió por I2C con 16 bytes completos; los segundos avanzaban pero la fecha era 01/01/2000 y el registro de estado era 0x88 (OSF activo). Se cargó firmware con diagnóstico y ajuste por USB. Con el ciclo en SIN_CICLO se ajustó a 30/10/2026 11:55:45, hora de Colombia. El equipo confirmó AJUSTE USB OK y RELOJ OK. Lecturas posteriores indicaron presente=1, valido=1, salto=0, registro de estado 0x08 y avance de segundos. No se verificó todavía la conservación de hora con alimentación principal desconectada.

La interpretación de OSF corresponde al [registro de estado del DS3231, fabricante Analog Devices](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf).

## Habilitación autorizada por el operador

Tras la confirmación explícita del propietario, se cargó el soporte USB y se guardó hardwareConfirmed=true. La lectura posterior del equipo confirmó storageOK=true, sensorOK=true, rtcOK=true y alarmas vacías. El estado permaneció SIN_CICLO y los cuatro relés apagados, coherente con idlePolicy=OFF. La habilitación no equivale a iniciar un ciclo ni constituye una inspección física del cableado por el software.

## Polaridad HIGH activa — 02/11/2026

A solicitud del propietario se cambiaron los cuatro valores iniciales activeLow a false y se agregó una migración única de la configuración NVS antes de inicializar los GPIO. Se conserva el ciclo y los parámetros existentes. Compilación, pruebas de lógica y carga USB correctas. La consulta posterior confirmó activeLow=false en los cuatro relés, storageOK=true, sensorOK=true y rtcOK=true. En INCUBANDO, la calefacción demandada estaba en HIGH; motor, ventilador y humidificador apagados estaban en LOW según lectura digital de GPIO. Se observó alarma de temperatura fuera del rango objetivo. Estas lecturas no equivalen a una medición externa de contactos o tensión.

## Conmutación de ventilador y DHT22 — 11/11/2026

Se capturó por USB una prueba de ventilador de 10 s sin fallo de sensor y una segunda activación seguida de ERROR SENSOR y apagado por protección. El operador informa carga de 110 V AC, relé alimentado por fuente externa de 5 V y masa compartida con ESP32. La coincidencia temporal no determina por sí sola si existe interferencia o caída de alimentación.

Se añadió una ventana de 250 ms desde el último cambio de relé antes de iniciar una lectura DHT22. No bloquea la tarea de control, no actualiza la antigüedad de lecturas al aplazar y no convierte fallos en datos válidos. Se mantienen muestreo mínimo de 2,5 s, invalidación inmediata ante lectura fallida, caducidad de 7 s, umbrales, polaridad, tiempos de pruebas y protecciones. Es una mitigación de adquisición durante el transitorio, no una certificación de eliminación de ruido eléctrico.

Pruebas nativas y compilación correctas. Un ensayo del módulo Sensors con DHT simulado verificó aplazamiento, intervalo mínimo, fallo inmediato, recuperación y caducidad sin renovación artificial. Registro de reproducción previo: tmp/diagnostico/ventilador_sensor_2026-11-11.jsonl. La validación con la carga real después de la actualización queda pendiente.

La carga USB de esta mitigación finalizó con hash verificado. En las tres consultas posteriores al arranque, rtcOK y storageOK fueron verdaderos, pero sensorOK siguió falso, sin una lectura válida inicial. Por tanto, el fallo físico observado NO quedó resuelto por esta actualización; se requiere comprobar alimentación y señal del DHT22 y el efecto de conmutar la carga AC. No se relajaron los bloqueos para forzar el ventilador.

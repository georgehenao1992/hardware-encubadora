# Errores y soluciones

| Síntoma | Comprobación / solución |
|---|---|
| No aparece WiFi | Alimentación estable, placa correcta, monitor 115200, ERROR WIFI; reiniciar con cargas desconectadas |
| WiFi sin Internet | Es normal; mantener conexión y abrir http://192.168.4.1 |
| No conozco claves | USB, monitor 115200, escribir `credenciales` y Enter; la clave inicial de esta versión es `CAMBIA_ESTA_CLAVE` para ambos accesos |
| 401 | Usuario admin; ambas claves iniciales son `CAMBIA_ESTA_CLAVE`, salvo cambios posteriores |
| 429 | Diez errores de autenticación; esperar un minuto |
| 403 | Usar IP exacta, UI del equipo y cabeceras requeridas, sin proxy |
| Panel ausente / 503 | Cargar LittleFS con uploadfs; verificar mapa de particiones |
| ERROR SENSOR | DHT22, VCC 3,3 V, masa, GPIO4, pull-up, cables, condensación y alimentación |
| Lecturas fuera de objetivo | Revisar capacidad térmica, humidificador, ubicación y calibración; no confundir con error de comunicación |
| ERROR RTC | Cableado I2C, dirección 0x68, batería, bandera lostPower; pausar y ajustar hora |
| Pausa tras pérdida de reloj | Revisar acumulado; solo reanudar con hora y duración verificadas |
| LCD vacía | Contraste, alimentación, adaptación de niveles, escáner y dirección 0x27/0x3F; reiniciar tras cambio |
| LCD filas 3/4 corridas | Verificar geometría 16×4 y compatibilidad de backpack |
| Relés no actúan | Hardware pendiente, estado seguro, GPIO, polaridad, prueba bloqueada por umbral o tiempo mínimo |
| Relés actúan al reset | Corregir eléctricamente polarización/driver; no se resuelve con retrasos en setup |
| Motor no vuelve inmediatamente tras corte | Intencional: ranuras vencidas e intentos inciertos se omiten |
| MOTOR INCIERTO | Inspeccionar posición mecánica; no se repite automáticamente el intento anterior |
| Motor se corta antes de duración | Entrada a nacimiento, pausa, sensor/reloj inválido, sobretemperatura o fallo NVS |
| Configuración rechazada | Finalizar ciclo; comprobar límites, tipos y relación entre umbrales de ventilador/calefacción |
| ERROR_CRITICO / NVS | Cargas desconectadas, mantenimiento por USB; no sobrescribir automáticamente estado corrupto |
| Reinicios con cargas | Fuente insuficiente, interferencia, masa/aislamiento, supresión inductiva, brownout o watchdog |
| Flash casi llena al compilar | Firmware usa ~94% de partición de aplicación estándar; no agregar dependencias pesadas sin revisar tamaño |
| Instalador sin espacio | Liberar espacio de forma controlada o usar herramientas ya instaladas; el proyecto fija versiones |

El historial contiene hasta 48 entradas y no sobrevive al reinicio; NVS conserva solamente el estado operativo importante. La ausencia de eventos viejos tras un corte no significa que no ocurrieran.

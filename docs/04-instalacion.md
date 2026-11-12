# Instalación y puesta en marcha

## PlatformIO (ruta verificada)

Abrir esta carpeta como proyecto PlatformIO. `platformio.ini` fija Espressif32 8.0.0 y las bibliotecas. `pio run -e esp32dev` instala dependencias si faltan y compila con Arduino ESP32 3.3.12. Se necesita Internet solo para instalar herramientas; el equipo funciona completamente sin Internet después de cargarlo.

Si `pio` no está en PATH, en este equipo se encuentra en `/Users/jorgehenao/.platformio/penv/bin/pio`. No cambiar permisos globales ni borrar directorios de herramientas ante un error de espacio: verificar espacio disponible primero.

```sh
pio run -e esp32dev
pio run -e esp32dev -t buildfs
./scripts/test.sh
```

Se generan `.pio/build/esp32dev/firmware.bin` y `littlefs.bin`, más bootloader, particiones y ELF. No flashear `firmware.bin` en dirección cero: usar PlatformIO para aplicar las direcciones correctas.

## Cargar en el equipo

1. Desconectar alimentación de las cargas. Verificar reposo físico de los relés con ESP32 apagada.
2. Conectar ESP32 por USB con cable de datos. Seleccionar puerto si hay varios (`pio device list`).
3. Ejecutar `pio run -e esp32dev -t upload`. Si no entra en bootloader, seguir secuencia BOOT/EN de la placa.
4. Ejecutar `pio run -e esp32dev -t uploadfs`. El firmware no formatea automáticamente LittleFS si falla el montaje.
5. Abrir `pio device monitor -b 115200`; escribir `credenciales` y Enter para recuperar las claves privadas por USB. Guardarlas fuera del repositorio. El monitor debe enviar fin de línea.
6. Verificar escáner I2C: DS3231 0x68 y LCD 0x27/0x3F. Revisar la causa de reinicio.
7. Conectar celular, ajustar RTC, esperar al menos una lectura válida de DHT22 y configurar políticas.
8. Con cargas todavía desconectadas, verificar niveles OFF y polaridad canal por canal. Confirmar instalación en el panel, guardar y ejecutar pruebas de baja tensión.
9. Solo después del protocolo completo conectar las cargas operativas.

`uploadfs` reemplaza el sistema de archivos web; no contiene NVS. La carga normal de firmware conserva NVS si el mapa de particiones no cambia. No borrar flash ni actualizar tablas de particiones durante un ciclo. Si cambia de modo simulación a producción, reiniciar con cargas desconectadas y revisar el estado real recuperado antes de alimentarlas.

## Arduino IDE

Se suministra un exportador que conserva exactamente la misma implementación:

```sh
python3 scripts/export_arduino.py
```

Abrir `dist/MiniGranjaPaulett/MiniGranjaPaulett.ino`. En el gestor de placas instalar **esp32 by Espressif Systems 3.3.12**; seleccionar **ESP32 Dev Module**, flash de 4 MB y esquema **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)** compatible con `default.csv` del core. El nombre de partición SPIFFS no impide usar LittleFS sobre ella.

En el gestor de bibliotecas instalar las versiones:

- RTClib de Adafruit 2.1.4 y Adafruit BusIO (dependencia).
- DHT sensor library de Adafruit 1.4.6.
- Adafruit Unified Sensor 1.1.15.
- LiquidCrystal I2C de Frank de Brabander / repositorio marcoschwartz 1.1.4.
- ArduinoJson de Benoit Blanchon 6.21.5.

WiFi, WebServer, Wire, Preferences, LittleFS, FreeRTOS y esp_timer pertenecen al core. Evitar instalar otras bibliotecas con los mismos nombres. Compilar y subir desde IDE. Para LittleFS, la ruta reproducible del proyecto es `pio run -e esp32dev -t uploadfs`, manteniendo el mismo esquema de particiones. La exportación se regenera después de cambios en `src/`; no editar la copia en paralelo. La compilación comprobada aquí es PlatformIO, no una sesión de Arduino IDE.

## Conectar el celular

1. Elegir red **MINI_GRANJA_PAULETT** y usar la clave WiFi del monitor serie.
2. Aceptar permanecer conectado aunque el teléfono indique «sin Internet»; desactivar temporalmente cambio automático a datos si redirige la conexión.
3. Abrir exactamente **http://192.168.4.1**, sin HTTPS. No se implementa portal cautivo/DNS.
4. Introducir clave de administrador (usuario `admin`, enviado por la interfaz).
5. Ajustar hora local del DS3231, revisar sensor, políticas y la instalación.
6. Guardar parámetros antes de iniciar. Inicio por defecto: hora del RTC al abrir panel; actualizarla si permaneció mucho tiempo abierta.
7. Iniciar con confirmación. Cerrar el navegador no detiene el sistema.

La interfaz consulta datos cada 2,5 s; si pierde conexión identifica datos antiguos. Al volver se consulta el estado real. Los eventos desaparecen al reiniciar porque viven en RAM. El estado persistido y las claves permanecen.

## Mantenimiento

El comando USB `credenciales` no borra configuraciones. Si NVS es corrupta o cambia el esquema, el equipo bloquea salidas. Registrar las fechas y parámetros, desconectar cargas y hacer mantenimiento local. Un borrado completo de flash elimina ciclos, claves y web, por lo que exige reprovisionar y recargar ambos artefactos; no se ofrece como acción desde el panel para evitar borrados accidentales.

## Diagnóstico y ajuste del RTC por USB

El firmware admite `rtc` en el monitor serie para mostrar presencia, validez, saltos, hora y los 16 primeros registros del DS3231. El comando `hora:NUMERO` ajusta una fecha local codificada como epoch; pasa por las mismas protecciones del panel y rechaza cambios durante un ciclo en marcha.

Para evitar calcular esa representación manualmente, cerrar el monitor serie y ejecutar:

```sh
~/.platformio/penv/bin/python scripts/rtc_usb.py
~/.platformio/penv/bin/python scripts/rtc_usb.py --set-time
```

El segundo comando copia la fecha/hora actuales de America/Bogota, comprueba la confirmación y realiza tres lecturas posteriores. No reanuda un ciclo. Se puede indicar otro puerto con `--port`. Algunos adaptadores USB reinician la ESP32 al abrir el puerto, incluso sin pedir un reset explícito.

## Confirmación de instalación por USB

`estado` muestra el estado operativo en JSON, sin contraseñas. `confirmar instalacion` guarda hardwareConfirmed=true después de la revisión eléctrica del operador. Conserva los demás parámetros y pasa por las mismas validaciones del panel; no permite cambios con un ciclo existente ni inicia un ciclo o pruebas manuales. Ambos comandos requieren acceso físico al puerto USB.

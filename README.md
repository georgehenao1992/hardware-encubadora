# MINI GRANJA PAULETT

Control autónomo de incubadora de gallina con ESP32 DevKit clásica, DHT22, DS3231, LCD 20×4 y cuatro relés. Interfaz en español, adaptable al celular y sin servicios externos. El proyecto conserva la estructura PlatformIO de esta carpeta y utiliza el framework Arduino.

## Credenciales de la versión publicada

`CAMBIA_ESTA_CLAVE` es un valor de ejemplo. Cámbialo antes de cargar firmware en un dispositivo nuevo; las contraseñas reales del montaje no se publican. Las menciones de contraseñas en el historial de documentación se han sustituido por ese valor. El informe institucional con datos personales se conserva localmente y se excluye de Git.

## Estado del MVP

El firmware incluye un pulsador normalmente abierto entre GPIO32 y GND para activar el LCD durante 30 segundos. Los cuatro relés trabajan con HIGH para encender y LOW para apagar.

En las pruebas con el ventilador AC conectado se observó pérdida de comunicación del DHT22 al conmutar la carga. La ventana de adquisición de 250 ms tras cambios de relé es una mitigación y **no ha resuelto el fallo eléctrico observado**. Deben verificarse alimentación, señal, cableado y supresión de transitorios antes de operar con huevos. Las protecciones de sensor permanecen activas. Consultar [validación](docs/07-validacion.md).

Materiales de soporte: [manual visual](output/pdf/Manual_visual_Mini_Granja_Paulett.pdf), [infografía](output/pdf/Infografia_logica_MVP_Paulett.pdf).

**Las salidas están bloqueadas de fábrica hasta confirmar la instalación eléctrica en el panel. No conectar cargas antes de leer [conexiones y seguridad](docs/02-conexiones.md).** La polaridad, tensión de entradas, dirección LCD y recorrido mecánico del motor deben verificarse en el equipo real.

## Entregables

1. [Arquitectura y reglas](docs/01-arquitectura.md).
2. [Conexiones y protecciones](docs/02-conexiones.md).
3. [Diagrama de estados](docs/01-arquitectura.md#estados).
4. [Parámetros y estructuras](docs/03-parametros.md).
5. Firmware modular en `src/`.
6. Interfaz local en `data/`.
7. NVS en `src/storage.cpp`, con esquema y verificación de integridad.
8. [Librerías y compilación](docs/04-instalacion.md).
9. [Carga de firmware y LittleFS](docs/04-instalacion.md#cargar-en-el-equipo).
10. [Conexión desde el celular](docs/04-instalacion.md#conectar-el-celular).
11. [Pruebas de relés](docs/05-pruebas.md#relés).
12. [Pruebas de sensores](docs/05-pruebas.md#sensores-y-reloj).
13. [Simulación de días 1, 18 y 21](docs/05-pruebas.md#simulación-acelerada).
14. [Cortes eléctricos y recuperación](docs/05-pruebas.md#recuperación).
15. [Solución de errores](docs/06-errores.md).
16. [Lista previa a huevos reales](docs/05-pruebas.md#lista-previa-a-operar).

## Comandos

```sh
pio run -e esp32dev                 # firmware de producción
pio run -e esp32dev -t buildfs      # imagen LittleFS
./scripts/test.sh                  # lógica C++ y sintaxis JavaScript
python3 scripts/export_arduino.py  # dist/MiniGranjaPaulett para Arduino IDE
```

Para cargar, con el equipo conectado y las cargas desconectadas:

```sh
pio run -e esp32dev -t upload
pio run -e esp32dev -t uploadfs
pio device monitor -b 115200
```

Abra `http://192.168.4.1` estando conectado a `MINI_GRANJA_PAULETT`. Por solicitud del propietario, la clave inicial WiFi y de administrador es **`CAMBIA_ESTA_CLAVE`**. Esta versión también actualiza las claves previamente guardadas una sola vez al arrancar, sin borrar el ciclo ni otros parámetros. Los cambios posteriores desde el panel se conservan. Si no alcanzó a verlas, escriba **`credenciales`** seguido de Enter en el monitor serie. Usuario administrativo: **admin**. No se requiere conexión a Internet en operación.

## Versiones

- PlatformIO Espressif32 **8.0.0**, Arduino ESP32 **3.3.12**, ESP-IDF **5.5.5**.
- RTClib 2.1.4, DHT sensor library 1.4.6, Adafruit Unified Sensor 1.1.15, LiquidCrystal_I2C 1.1.4, ArduinoJson 6.21.5.
- Dependencias y bibliotecas del core declaradas en `platformio.ini`.

La relación entre plataforma y core está documentada en la [publicación oficial de PlatformIO 8.0.0](https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0). La selección de LittleFS sigue la [documentación de PlatformIO](https://docs.platformio.org/en/latest/platforms/espressif32.html#using-filesystem). El watchdog utiliza la [API de ESP-IDF 5.5](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32/api-reference/system/wdts.html).

Consulte el [resultado detallado de validación](docs/07-validacion.md), incluidas las comprobaciones pendientes y la vista local de diseño.

Las pruebas de software y compilación no certifican una instalación eléctrica ni el desempeño térmico. Las pruebas físicas del protocolo deben completarse antes de operar con huevos.

La actualización del 02/11/2026 configura los cuatro relés con **HIGH = encendido / LOW = apagado**. Al primer arranque migra también la polaridad guardada en NVS, antes de habilitar GPIO, sin borrar los demás parámetros ni el ciclo.

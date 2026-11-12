# Conexiones y seguridad eléctrica

## Asignación obligatoria

| Componente | Señal | GPIO ESP32 |
|---|---|---|
| LCD 20×4 I2C | SDA | 21 |
| LCD 20×4 I2C | SCL | 22 |
| DS3231 | SDA | 21 |
| DS3231 | SCL | 22 |
| DHT22 | DATA | 4 |
| Relé 1 – Motor | IN1 | 25 |
| Relé 2 – Calefacción | IN2 | 26 |
| Relé 3 – Ventilador | IN3 | 27 |
| Relé 4 – Humidificador | IN4 | 33 |
| Pulsador de pantalla (normalmente abierto) | Contacto a GND | 32 |

Usar numeración **GPIO**, no posición física del conector. La placa es ESP32 clásica, no ESP32-S2/S3/C3. Confirmar 4 MB de flash para el perfil `esp32dev`.

## Pulsador de pantalla (actualización 04/11/2026)

Conectar con el equipo sin alimentación un pulsador momentáneo **normalmente abierto** entre **GPIO32** (marcado 32 o IO32, no posición 32 del conector) y **GND de la ESP32**:

```text
GPIO32 ───── pulsador normalmente abierto ───── GND
```

Se usa `INPUT_PULLUP`: en reposo HIGH, al pulsar LOW. Para un cable corto no hace falta resistencia externa. No conectar el pulsador a 5 V ni a los contactos de potencia de los relés. Si tiene cuatro patas, usar dos contactos que estén abiertos en reposo y se unan al pulsar; comprobarlo con continuidad porque las patas de cada pareja pueden estar unidas internamente. GPIO32 admite entrada con pull-up según la [documentación de Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html).

La pantalla inicia sin iluminación y sin caracteres visibles. Una pulsación estable de al menos 40 ms muestra los valores actuales durante 30 segundos; otra pulsación reinicia el plazo. Mantener el botón apretado no prolonga el encendido. Se apagan la retroiluminación y los caracteres mediante comandos I2C; el módulo conserva su alimentación. El ahorro principal es la luz del LCD, no se suspende la ESP32. Sensores, reloj, aplicación, alarmas y cuatro relés conservan su funcionamiento. Las alarmas no encienden automáticamente la pantalla.

Prueba física: tras reiniciar debe quedar oscura; pulsar y comprobar las cuatro filas; esperar 30 segundos y comprobar el apagado; volver a pulsar durante el plazo para comprobar que comienza otro intervalo. Verificar en la app que el control sigue activo mientras la pantalla está apagada.

## Datos por confirmar antes de habilitar

- Modelo exacto de módulo de relés, entradas compatibles con 3,3 V, consumo, aislamiento y polaridad real de cada canal. La configuración solicitada el 02/11/2026 es HIGH activo y LOW apagado en los cuatro canales. Verificar compatibilidad física y reposo LOW mediante el driver/polarización externa adecuados.
- Tensiones, corriente nominal y de arranque de cada carga; capacidad de contactos para cargas inductivas y tipo AC/DC.
- Tiempo mecánico necesario para un volteo, posibilidad de atasco y necesidad de final de carrera. El valor inicial 10 s es ilustrativo; no implica un ángulo conocido.
- LCD: dirección 0x27 o 0x3F, alimentación, mapeo PCF8574 y filas 20×4. El escáner de arranque muestra las direcciones por serie y evento I2C.
- DS3231: alimentación del módulo, resistencias pull-up y tipo de batería. Algunos módulos traen cargador; **no cargar una CR2032 no recargable**. Revisar el circuito del módulo.
- Determinar si ventilar durante un fallo de sensor reduce el riesgo de la instalación; política inicial OFF.

## Niveles y reposo

Los GPIO del ESP32 son de **3,3 V**. No conectar pull-ups I2C a 5 V directamente. Para un LCD a 5 V con pull-ups a 5 V, usar adaptador bidireccional apropiado para I2C: lado LV a 3,3 V / ESP32 y lado HV a 5 V / LCD. El DS3231 puede quedar en el lado 3,3 V si su módulo lo admite. Compartir SDA/SCL y referencia de masa de la electrónica según el aislamiento elegido. Revisar resistencias combinadas del bus; no asumir que todos los módulos tienen la misma topología.

DHT22 a 3,3 V con DATA a GPIO4 y pull-up, típicamente 4,7–10 kΩ a 3,3 V según longitud de cable y módulo. No agregar resistencias sin comprobar las que trae el sensor. Mantener cableado corto, desacoplar alimentación y separar de cables de potencia.

En reset, bootloader y antes de confirmar hardware, los GPIO de relés quedan en alta impedancia. **El software no puede garantizar reposo eléctrico durante esos intervalos.** Se requieren pull-ups o pull-downs externos y, cuando sea necesario, transistor/optoacoplador/driver para garantizar OFF con ESP32 apagada o reiniciando. Para activo LOW, garantizar IN alto de forma compatible con 3,3 V; para activo HIGH, garantizar IN bajo. No llevar 5 V al GPIO para obtener el reposo. Medir con cargas desconectadas, incluyendo pérdida de alimentación de ESP32 con el módulo de relés todavía encendido.

Una vez confirmada la polaridad, el firmware precarga el nivel OFF antes de configurar OUTPUT. Cambiar polaridad solamente con alimentación de cargas desconectada. Durante el cambio pueden variar los niveles físicos, por lo que no basta con el bloqueo lógico de interfaz.

## Potencia

- GPIO únicamente a entradas de driver compatibles; nunca alimentar bobinas, motores, ventilador o humidificador desde un GPIO.
- Alimentación externa dimensionada para relés y cargas, con referencia común cuando la interfaz lo requiera. Si usa optoaislamiento real, revisar JD-VCC y masas conforme al esquema del fabricante, sin puentear por costumbre.
- Contactos normalmente abiertos para que la pérdida de energía deje cargas apagadas. Relés con valores nominales adecuados al arranque del motor y a la carga térmica.
- Fusible apropiado por circuito, puesta a tierra de partes metálicas, aislamiento, borneras protegidas, separación física de potencia y sensores. No montar tensión de red en protoboard.
- Supresión adecuada a cada carga: diodo para bobinas DC donde corresponda; snubber/MOV para cargas AC según diseño. No colocar un diodo de rueda libre sobre una carga AC.
- **Termostato o limitador térmico independiente en serie con la calefacción**, de corte físico, y fusible térmico cuando corresponda. El firmware no protege contra un relé soldado.
- El ventilador térmico no sustituye el aporte permanente de aire fresco que requieran los embriones. Dimensionar circulación y renovación por separado.

Antes de huevos reales, verificar con instrumentos de referencia la uniformidad térmica, humedad, respuesta y desfase del DHT22. Los valores predeterminados son ejemplos de configuración, no un protocolo biológico validado para su incubadora.

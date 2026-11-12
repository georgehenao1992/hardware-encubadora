# Parámetros, estructuras y API

`src/config.h` es la fuente de verdad. `Config` contiene ambas etapas, parámetros del motor, políticas, cuatro polaridades, confirmación física, dirección LCD y claves. `Cycle` contiene estado, inicio, ancla de reanudación, tiempo acumulado, pausas, próxima ranura y últimos intentos. `Sensors` conserva último valor válido, antigüedad y resultado de la lectura. Cada relé expone AUTOMATICO, MANUAL_TEMPORAL, DESHABILITADO o BLOQUEADO_POR_SEGURIDAD. ERROR_CRITICO se usa para fallos de almacenamiento/temporizador; los errores recuperables se muestran como alarmas y pausa segura cuando corresponde.

| Parámetro | Inicial | Validación |
|---|---:|---|
| Calefacción ON / OFF incubación | 37,5 / 37,8 °C | ON ≥30, OFF ≤39, ON < OFF |
| Ventilador ON / OFF incubación | 38,0 / 37,8 °C | OFF ≥ heatOff, ON > OFF, ON ≤40 |
| Humedad ON / OFF incubación | 50 / 60 % | ON ≥20, OFF ≤85, ON < OFF |
| Calefacción nacimiento | 37,2 / 37,5 °C | Igual regla |
| Ventilador nacimiento | 37,8 / 37,5 °C | Igual regla |
| Humedad nacimiento | 65 / 70 % | Igual regla |
| Sobretemperatura | 39,5 °C | Mayor a fanOn de ambas etapas, ≤42 |
| Duración motor | 10 s | Entero 1–máximo configurado |
| Máximo motor | 30 s | Entero 1–120 s absolutos |
| Intervalo motor | 10.800 s / 180 min | Entero 600–86.400 s |
| Primer movimiento | 0 = un intervalo tras inicio | Si explícito, dentro de las primeras 432 h |
| Volteo automático | Habilitado | No elude etapa ni seguridad |
| Fin a 18 días | No | Sí: 432 h; no: 504 h |
| Política fuera de ciclo | OFF | OFF o REGULATE |
| Ventilador con sensor inválido | OFF | OFF o ON |
| Hardware confirmado | No | Requiere revisión física |
| Activo LOW por canal | No: encendido HIGH, apagado LOW | Cuatro booleanos |
| LCD | 0x27 | 0x27 o 0x3F; cambio aplica al reiniciar |
| Lectura DHT22 | Cada 2.500 ms | T 0–60 °C; HR 1–100 % |
| Máxima antigüedad de lectura | 7.000 ms | Una lectura inválida bloquea antes |
| Cambio de relé | 5.000 ms | Encendidos restringidos; apagado seguro inmediato |
| Historial | 48 eventos | RAM circular |

Los parámetros no son modificables durante un ciclo, incluyendo pausa: finalizar antes de guardar evita cambios inconsistentes de duración o programación. La duración se deriva de la política persistida 18/21 días; no se permite una duración arbitraria. No hay modo manual indefinido. Las pruebas ambientales aceptan 1–30 s en API, 10 s en interfaz, y no pueden extenderse mientras estén activas; al terminar vuelven al control automático que puede seguir demandando la salida.

## HTTP

Todo `/api/*` requiere Basic `admin:clave` y Host `192.168.4.1`. `GET /api/status` retorna reloj, calidad de sensor, ciclo, tiempos, modos/salidas, alarmas y eventos. `GET /api/config` devuelve parámetros sin contraseñas. `POST /api/command` exige `Content-Type: application/json`, `X-Paulett-Request: 1` y `confirm:true`. Todos los campos se validan también en ESP32. Cuerpos de más de 4.096 caracteres se rechazan en el manejador; el servidor base puede haberlos recibido antes de esa comprobación.

| action | Campos |
|---|---|
| start / new | start: segundos de fecha local codificada como epoch |
| pause / resume / finish | sin parámetros adicionales |
| clock | epoch: fecha local 2025–2099; no durante marcha |
| test | relay: 0–3; seconds: 1–30 para ambientales; motor usa configuración |
| stopTests | termina pruebas, devuelve control automático |
| config | Config completa sin credenciales ni schema; ver GET |
| credentials | wifi: ASCII 8–63, admin: ASCII 8–63, sin espacios |
| simulate | elapsed: 0–1.814.400; solo compilación de pruebas |

Las fechas son una representación numérica de hora local, no UTC geográfica. El navegador codifica componentes de fecha con sufijo Z y presenta usando UTC para evitar conversiones dobles; «usar hora del teléfono» copia sus componentes locales. Elegir zona del teléfono correcta antes de copiar. No cambiar fecha externamente con un ciclo en marcha.

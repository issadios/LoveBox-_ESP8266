# 💌 LoveBox — Versión Tamagotchi

> Un buzón de mensajes digital y mascota virtual construido con ESP8266, conectado a Telegram, con animaciones en pantalla OLED y sonidos expresivos.

Este dispositivo está pensado como un regalo especial: permite enviar mensajes de texto y estados de ánimo desde cualquier parte del mundo hacia la pantalla de la LoveBox. Al recibir un mensaje, la caja avisa mediante animaciones y sonidos. El receptor puede responder con un solo toque físico.

---

## ✨ Características

- **Recepción de mensajes** — Muestra mensajes de texto enviados desde Telegram en la pantalla OLED.
- **Mascota virtual animada** — Carita expresiva con 6 estados de ánimo, cada uno con animación propia en múltiples frames.
- **Sonidos únicos por emoción** — Cada estado tiene su melodía característica (vals romántico, glissando de sorpresa, suspiro triste, etc.).
- **Corazoncitos y efectos flotantes** — Corazones que suben, letras "Zzz" que flotan, estrellitas parpadeantes y lágrimas animadas.
- **Auto-retorno a reposo** — La mascota vuelve a su estado idle automáticamente después de 60 segundos.
- **Botón físico** — Al presionarlo, la LoveBox envía un mensaje automático de vuelta por Telegram.
- **Reloj digital** — Sincronización automática de hora vía NTP (configurado para México, UTC-6).
- **Carcasa imprimible en 3D** — Archivos STL incluidos en el repositorio.

---

## 🎭 Estados de ánimo y animaciones

| Comando       | Animación en pantalla                        | Sonido                        |
|---------------|----------------------------------------------|-------------------------------|
| `/feliz`      | Carita brinca entre 2 frames + estrellitas   | Do-Mi-Sol-Do ascendente       |
| `/triste`     | Lágrima que cae pixel a pixel                | 3 notas descendentes (suspiro)|
| `/enamorado`  | Guiño alterno + corazoncito flotante         | Vals corto de 6 notas         |
| `/dormido`    | Ojos cerrados + "z Z" que sube flotando      | Dos notas suaves              |
| `/sorprendido`| Cejas saltando + líneas de impacto           | Glissando 300 → 1200 Hz       |
| *(reposo)*    | Parpadeo natural cada ~8 segundos            | —                             |

Al recibir cualquier mensaje de texto libre, se muestra en pantalla con una animación de corazones y una melodía de notificación.

---

## 🛒 Hardware necesario

| Componente | Descripción |
|---|---|
| Microcontrolador | ESP8266 — NodeMCU V3, Wemos D1 Mini o similar |
| Pantalla | OLED 128×64 px, controlador SSD1306, interfaz SPI |
| Sonido | Buzzer pasivo (recomendado) o activo |
| Entrada | Botón pulsador (push button) |
| Varios | Cables jumper Dupont, protoboard o placa perforada |
| Carcasa | Filamento PLA, PETG o ABS para impresión 3D |

---

## 🔌 Conexiones de hardware

La pantalla usa **Software SPI**. Conectar al ESP8266 de la siguiente manera:

| Componente    | Pin del componente | GPIO ESP8266 | Pin NodeMCU |
|---------------|--------------------|--------------|-------------|
| Pantalla OLED | DIN (MOSI)         | GPIO 13      | D7          |
| Pantalla OLED | CLK                | GPIO 14      | D5          |
| Pantalla OLED | DC                 | GPIO 4       | D2          |
| Pantalla OLED | CS                 | GPIO 15      | D8          |
| Pantalla OLED | RES                | GPIO 0       | D3          |
| Buzzer        | Positivo (+)       | GPIO 5       | D1          |
| Botón         | Terminal 1         | GPIO 2       | D4          |
| Botón         | Terminal 2         | GND          | GND         |

> **Nota:** El botón usa la resistencia pull-up interna (`INPUT_PULLUP`), por lo que se conecta directamente a GND sin resistencia externa.

---

## 💻 Requisitos de software

El código está escrito para **Arduino IDE**. Instala las siguientes librerías desde el Gestor de Librerías:

- `ESP8266WiFi` — incluida en el núcleo del ESP8266
- `WiFiClientSecure` — incluida en el núcleo del ESP8266
- `UniversalTelegramBot` — por Brian Lough
- `ArduinoJson` — requerida por UniversalTelegramBot
- `Adafruit GFX Library`
- `Adafruit SSD1306`

Para instalar el núcleo del ESP8266 en Arduino IDE, agrega esta URL en *Preferencias → Gestor de URLs adicionales*:

```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

---

## ⚙️ Configuración paso a paso

### 1. Crear el bot de Telegram

1. Abre Telegram y busca **@BotFather**.
2. Envía `/newbot` y sigue las instrucciones.
3. Copia el **token** que te proporciona (formato: `123456789:AABBcc...`).
4. Para obtener tu **Chat ID**, busca el bot **@userinfobot** y envíale cualquier mensaje.

### 2. Configurar el código

Abre el archivo `.ino` y modifica estas líneas al principio:

```cpp
const char* ssid     = "TU_RED_WIFI";
const char* password = "TU_CONTRASEÑA";

#define BOTtoken "TU_TOKEN_DE_TELEGRAM"
#define CHAT_ID  "TU_CHAT_ID"
```

> ⚠️ **Importante:** Nunca subas el archivo con tus credenciales reales al repositorio. Usa el archivo `.gitignore` incluido o reemplaza los valores antes de hacer commit.

### 3. Parámetros ajustables

Al inicio del archivo puedes modificar estos valores según tu preferencia:

```cpp
const long ANIMATION_DELAY     = 200;    // ms entre frames (menor = más rápido)
const long MESSAGE_VIEW_DURATION = 12000; // ms que se muestra un mensaje
const long EMOTION_DURATION    = 60000;  // ms antes de volver a reposo
```

### 4. Subir el código

1. Selecciona tu placa en *Herramientas → Placa → NodeMCU 1.0* (o la que uses).
2. Selecciona el puerto correcto.
3. Sube el sketch con el botón de carga.

---

## 📱 Comandos de Telegram

| Comando        | Efecto |
|----------------|--------|
| `/start`       | Muestra el mensaje de bienvenida y lista los comandos |
| `/feliz`       | Pone a la mascota en estado feliz |
| `/triste`      | Pone a la mascota en estado triste |
| `/enamorado`   | Pone a la mascota en estado enamorado |
| `/dormido`     | Pone a la mascota en estado dormido |
| `/sorprendido` | Pone a la mascota en estado sorprendido |
| *Texto libre*  | Se muestra como mensaje en la pantalla con animación y sonido |

---

## 🖨️ Carcasa 3D

Los archivos STL de la carcasa están en la carpeta `/carcasa` del repositorio.

El diseño está pensado para:
- Ajustarse a la pantalla OLED en la parte frontal.
- Mantener el ESP8266 seguro en el interior.
- Dejar aberturas para el botón y salida de sonido del buzzer.

**Recomendaciones de impresión:**

| Parámetro    | Valor recomendado |
|--------------|-------------------|
| Altura de capa | 0.2 mm |
| Relleno | 15–20% |
| Material | PLA o PETG |
| Soportes | Solo si la orientación lo requiere |

---


## 🔒 Seguridad

```cpp
// credentials.h  
#define WIFI_SSID     "tu_red"
#define WIFI_PASSWORD "tu_contraseña"
#define BOT_TOKEN     "tu_token"
#define BOT_CHAT_ID   "tu_chat_id"
```

---

## 🧑‍💻 Créditos y librerías utilizadas

- [UniversalTelegramBot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot) — Brian Lough
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) — Adafruit Industries
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) — Adafruit Industries
- [ArduinoJson](https://arduinojson.org/) — Benoit Blanchon

---

## 📄 Licencia

Este proyecto se distribuye bajo la licencia **MIT**. Puedes usarlo, modificarlo y compartirlo libremente dando crédito al autor original.

---

*Hecho con  en Tuxtla Gutiérrez, Chiapas — IssaTech 3D*

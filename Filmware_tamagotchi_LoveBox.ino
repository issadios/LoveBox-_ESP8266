/*
  PROYECTO: LoveBox - Versión Tamagotchi (v2 - Animaciones mejoradas)
  MODO: SOFTWARE SPI + NTP + Máquina de Estados de Mascota
  MEJORAS v2:
    - Bitmaps 16x16 dibujados correctamente para cada emoción
    - Múltiples frames de animación por estado
    - Sonidos únicos y expresivos por emoción
    - Animación de "Zzz" flotante en estado dormido
    - Corazoncitos flotantes en estado enamorado
    - Animación de llegada de mensaje con efecto de typing
    - Auto-retorno a IDLE después de 60 segundos
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// ---------------- DATOS DE CONEXIÓN ---------------- //
const char* ssid     = "Mega-2.4G-92B1";
const char* password = "be3wz7qZ2d";

#define BOTtoken "8421539902:AAGoWrxUGEUY7upemKlt0EpkfWIzbDBUDUg"
#define CHAT_ID  "6811469342"

// ---------------- CONFIGURACIÓN PANTALLA (SOFTWARE SPI) ---------------- //
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

#define OLED_MOSI  13  // D7
#define OLED_CLK   14  // D5
#define OLED_DC     4  // D2
#define OLED_CS    15  // D8
#define OLED_RESET  0  // D3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

const int BUZZER_PIN = 5;  // D1
const int BUTTON_PIN = 2;  // D4

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// ---------------- VARIABLES DE CONTROL ---------------- //
const long BOT_DELAY          = 1000;
const long ANIMATION_DELAY    = 200;   // ms entre frames de animación
const long MESSAGE_VIEW_DURATION = 12000; // 12s para ver el mensaje
const long EMOTION_DURATION   = 60000;  // 60s antes de volver a IDLE

unsigned long lastTimeBotRan    = 0;
unsigned long lastTimeAnimation = 0;
unsigned long messageDisplayTime = 0;
unsigned long emotionStartTime  = 0;

String mensajeActual = "";
bool isTimeSynced = false;
int animFrame = 0;  // frame actual de animación (cicla cada ANIMATION_DELAY ms)

// Configuración Hora México (UTC-6)
const char* ntpServer      = "pool.ntp.org";
const long  gmtOffset_sec  = -21600;
const int   daylightOffset_sec = 0;

// ---------------- MÁQUINA DE ESTADOS ---------------- //
enum EstadoCarita { IDLE, FELIZ, TRISTE, ENAMORADO, DORMIDO, SORPRENDIDO };
EstadoCarita estadoCaritaActual = IDLE;

const int PET_W = 16;
const int PET_H = 16;
const int PET_SCALE = 2;  // → 32×32 en pantalla

// ================================================================
// BITMAPS 16×16 (32 bytes cada uno, 2 bytes por fila, MSB a la izq.)
// ================================================================
// Convención visual (cada bit = 1 pixel blanco):
//   Ojos: grupos de pixels en filas 4-7
//   Nariz/mejilla: fila 8-9 (opcional)
//   Boca: filas 9-12

// ---- IDLE (ojos abiertos, boca neutral) ----
const unsigned char PROGMEM pet_idle[] = {
  0x00,0x00,  // fila  0
  0x00,0x00,  // fila  1
  0x00,0x00,  // fila  2
  0x3C,0x3C,  // fila  3  ░░██████░░░░██████░░
  0x7E,0x7E,  // fila  4  ░███████░░░████████░
  0x7E,0x7E,  // fila  5
  0x3C,0x3C,  // fila  6
  0x00,0x00,  // fila  7
  0x00,0x00,  // fila  8
  0x18,0x18,  // fila  9  mejillas
  0x00,0x00,  // fila 10
  0x00,0x7E,  // fila 11  boca recta (lado derecho)
  0x3C,0x3C,  // fila 12
  0x00,0x00,  // fila 13
  0x00,0x00,  // fila 14
  0x00,0x00   // fila 15
};

// ---- BLINK (ojos cerrados = líneas) ----
const unsigned char PROGMEM pet_blink[] = {
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x7E,0x7E,  // ojos = línea horizontal
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x18,0x18,
  0x00,0x00,
  0x00,0x7E,
  0x3C,0x3C,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- FELIZ (ojos en arco, boca grande sonriente) ----
const unsigned char PROGMEM pet_feliz[] = {
  0x00,0x00,
  0x00,0x00,
  0x3C,0x3C,  // arcos de ojos (parte alta)
  0x7E,0x7E,
  0x66,0x66,  // ojos "^" (arco abierto hacia abajo)
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x42,0x42,  // comisuras boca
  0x66,0x66,
  0x3C,0x3C,  // boca grande curva
  0x18,0x18,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- FELIZ_BRINCA (ojos felices más pequeños, cejas alzadas) ----
const unsigned char PROGMEM pet_feliz_b[] = {
  0x00,0x00,
  0x66,0x66,  // cejas alzadas
  0x00,0x00,
  0x3C,0x3C,
  0x66,0x66,
  0x3C,0x3C,
  0x00,0x00,
  0x00,0x00,
  0x24,0x24,  // cachetes
  0x42,0x42,
  0x7E,0x7E,  // boca sonriente grande
  0x3C,0x3C,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- TRISTE (ojos caídos, boca invertida, lágrimas) ----
const unsigned char PROGMEM pet_triste[] = {
  0x00,0x00,
  0x00,0x00,
  0x66,0x66,  // cejas en V invertida (triste)
  0x3C,0x3C,
  0x3C,0x3C,
  0x3C,0x3C,
  0x00,0x00,
  0x24,0x24,  // lágrimas
  0x24,0x24,
  0x00,0x00,
  0x18,0x18,  // boca recta
  0x24,0x24,  // curva hacia abajo
  0x42,0x42,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- TRISTE_LLORA (lágrimas más grandes) ----
const unsigned char PROGMEM pet_triste_l[] = {
  0x00,0x00,
  0x66,0x66,
  0x42,0x42,  // cejas más caídas
  0x3C,0x3C,
  0x3C,0x3C,
  0x3C,0x3C,
  0x66,0x66,  // lágrimas más grandes
  0x66,0x66,
  0x24,0x24,
  0x00,0x00,
  0x18,0x18,
  0x24,0x24,
  0x42,0x42,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- ENAMORADO (ojos de corazón) ----
const unsigned char PROGMEM pet_enamorado[] = {
  0x00,0x00,
  0x00,0x00,
  0x36,0x36,  // corazón-ojo: parte superior (doble bump)
  0x7F,0x7F,
  0x7F,0x7F,
  0x3E,0x3E,
  0x1C,0x1C,
  0x08,0x08,  // punta del corazón-ojo
  0x00,0x00,
  0x42,0x42,  // comisuras
  0x66,0x66,
  0x3C,0x3C,  // boca sonriente enamorada
  0x18,0x18,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- ENAMORADO_WINK (guiño + corazón en un ojo) ----
const unsigned char PROGMEM pet_enamorado_w[] = {
  0x00,0x00,
  0x00,0x00,
  0x06,0x30,  // ojo izq: corazón pequeño, ojo der: arco
  0x0F,0x78,
  0x0F,0x78,
  0x06,0x30,
  0x00,0x00,  // guiño (ojo cerrado como ~)
  0x00,0x3C,
  0x00,0x00,
  0x42,0x42,
  0x66,0x66,
  0x3C,0x3C,
  0x18,0x18,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- DORMIDO (ojos cerrados en Z, boca pequeña) ----
const unsigned char PROGMEM pet_dormido[] = {
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x7E,0x7E,  // ojos cerrados (línea)
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x18,0x18,  // boca pequeña dormida
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- SORPRENDIDO (ojos abiertos grandes, boca en "O") ----
const unsigned char PROGMEM pet_sorprendido[] = {
  0x00,0x00,
  0x00,0x00,
  0x3C,0x3C,  // cejas alzadas de sorpresa
  0x00,0x00,
  0x7E,0x7E,  // ojos muy abiertos
  0xFF,0xFF,
  0x7E,0x7E,
  0x00,0x00,
  0x00,0x00,
  0x3C,0x3C,  // boca abierta (O)
  0x66,0x66,
  0x66,0x66,
  0x3C,0x3C,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ---- SORPRENDIDO_2 (cejas aún más arriba) ----
const unsigned char PROGMEM pet_sorprendido2[] = {
  0x3C,0x3C,  // cejas en lo más alto
  0x00,0x00,
  0x00,0x00,
  0x7E,0x7E,
  0xFF,0xFF,
  0xFF,0xFF,
  0x7E,0x7E,
  0x00,0x00,
  0x00,0x00,
  0x3C,0x3C,
  0x42,0x42,
  0x42,0x42,
  0x3C,0x3C,
  0x00,0x00,
  0x00,0x00,
  0x00,0x00
};

// ================================================================
// BITMAPS DE CORAZONES (32×32, 128 bytes)
// ================================================================
const unsigned char PROGMEM heart_small[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x07,0x00,
  0x01,0xF0,0x0F,0x80,0x03,0xF8,0x1F,0xC0,0x07,0xFC,0x3F,0xE0,0x07,0xFE,0x7F,0xE0,
  0x07,0xFE,0x7F,0xE0,0x07,0xFE,0x7F,0xE0,0x03,0xFC,0x3F,0xC0,0x01,0xF8,0x1F,0x80,
  0x00,0xF0,0x0F,0x00,0x00,0xE0,0x07,0x00,0x00,0x40,0x02,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const unsigned char PROGMEM heart_big[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0x00,
  0x03,0xFC,0x3F,0xC0,0x07,0xFE,0x7F,0xE0,0x0F,0xFF,0xFF,0xF0,0x1F,0xFF,0xFF,0xF8,
  0x1F,0xFF,0xFF,0xF8,0x3F,0xFF,0xFF,0xFC,0x3F,0xFF,0xFF,0xFC,0x3F,0xFF,0xFF,0xFC,
  0x3F,0xFF,0xFF,0xFC,0x1F,0xFF,0xFF,0xF8,0x1F,0xFF,0xFF,0xF8,0x0F,0xFF,0xFF,0xF0,
  0x07,0xFF,0xFF,0xE0,0x03,0xFF,0xFF,0xC0,0x01,0xFF,0xFF,0x80,0x00,0xFF,0xFF,0x00,
  0x00,0x7F,0xFE,0x00,0x00,0x3F,0xFC,0x00,0x00,0x1F,0xF8,0x00,0x00,0x0F,0xF0,0x00,
  0x00,0x03,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Corazón pequeño 8×8 para decoraciones
const unsigned char PROGMEM heart_tiny[] = {
  0x00, // 00000000
  0x36, // 00110110
  0x7F, // 01111111
  0x7F, // 01111111
  0x3E, // 00111110
  0x1C, // 00011100
  0x08, // 00001000
  0x00  // 00000000
};

// Estrella 8×8 para efecto de felicidad
const unsigned char PROGMEM star_tiny[] = {
  0x08, // 00001000
  0x1C, // 00011100
  0x08, // 00001000
  0x7F, // 01111111
  0x7F, // 01111111
  0x08, // 00001000
  0x1C, // 00011100
  0x08  // 00001000
};

// ================================================================
// UTILIDADES DE DIBUJO
// ================================================================

void drawScaledBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
    int16_t w, int16_t h, uint16_t color, uint8_t scale) {
  int16_t byteWidth = (w + 7) / 8;
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        display.fillRect(x + i * scale, y + j * scale, scale, scale, color);
      }
    }
  }
}

void displayStatus(String s1, String s2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(s1);
  if (s2 != "") {
    display.setCursor(0, 20);
    display.println(s2);
  }
  display.display();
}

void centerText(String text, int y, int sz = 1) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(sz);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(text);
}

// ================================================================
// SONIDOS EXPRESIVOS POR ESTADO
// ================================================================

void playNotificationSound() {
  // Melodía alegre de 3 notas ascendentes
  tone(BUZZER_PIN, 659, 80);  delay(90);
  tone(BUZZER_PIN, 784, 80);  delay(90);
  tone(BUZZER_PIN,1047, 150); delay(170);
  noTone(BUZZER_PIN);
}

void playSoundFeliz() {
  // Do-Mi-Sol-Do alegre y rápido
  int n[] = {523,659,784,1047};
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, n[i], 80); delay(100);
  }
  noTone(BUZZER_PIN);
}

void playSoundTriste() {
  // Notas descendentes lentas, como un suspiro
  tone(BUZZER_PIN, 494, 200); delay(230);
  tone(BUZZER_PIN, 440, 200); delay(230);
  tone(BUZZER_PIN, 392, 300); delay(320);
  noTone(BUZZER_PIN);
}

void playSoundEnamorado() {
  // Vals sencillo: tres notas románticas
  int n[] = {523,659,784,659,784,1047};
  int d[] = {120,120,200,120,120,300};
  for (int i = 0; i < 6; i++) {
    tone(BUZZER_PIN, n[i], d[i]); delay(d[i] + 20);
  }
  noTone(BUZZER_PIN);
}

void playSoundDormido() {
  // Dos notas suaves, como ronquido suave
  tone(BUZZER_PIN, 330, 200); delay(300);
  tone(BUZZER_PIN, 294, 300); delay(400);
  noTone(BUZZER_PIN);
}

void playSoundSorprendido() {
  // Glissando rápido hacia arriba
  for (int f = 300; f < 1200; f += 60) {
    tone(BUZZER_PIN, f, 20); delay(20);
  }
  noTone(BUZZER_PIN);
}

void playSoundBoton() {
  tone(BUZZER_PIN, 2000, 80);  delay(90);
  tone(BUZZER_PIN, 2500, 120); delay(130);
  noTone(BUZZER_PIN);
}

// ================================================================
// ANIMACIONES ESPECIALES
// ================================================================

// Corazones flotantes (para ENAMORADO y mensajes de amor)
void showHeartAnimation() {
  for (int i = 0; i < 4; i++) {
    display.clearDisplay();
    // Corazón grande centrado
    display.drawBitmap((SCREEN_WIDTH-32)/2, (SCREEN_HEIGHT-32)/2,
        heart_big, 32, 32, WHITE);
    // Corazoncitos pequeños alrededor
    if (i % 2 == 0) {
      display.drawBitmap(10, 10,  heart_tiny, 8, 8, WHITE);
      display.drawBitmap(110, 45, heart_tiny, 8, 8, WHITE);
    } else {
      display.drawBitmap(10, 45,  heart_tiny, 8, 8, WHITE);
      display.drawBitmap(110, 10, heart_tiny, 8, 8, WHITE);
    }
    display.display();
    tone(BUZZER_PIN, 200 + i * 50, 50);
    delay(180);
  }
}

// Estrellitas de felicidad
void showHappyAnimation() {
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    drawScaledBitmap(48, 16, pet_feliz, PET_W, PET_H, WHITE, PET_SCALE);
    if (i % 2 == 0) {
      display.drawBitmap(5,  20, star_tiny, 8, 8, WHITE);
      display.drawBitmap(115,20, star_tiny, 8, 8, WHITE);
      display.drawBitmap(5,  45, star_tiny, 8, 8, WHITE);
    } else {
      display.drawBitmap(15, 10, star_tiny, 8, 8, WHITE);
      display.drawBitmap(105,10, star_tiny, 8, 8, WHITE);
      display.drawBitmap(15, 50, star_tiny, 8, 8, WHITE);
    }
    display.display();
    tone(BUZZER_PIN, 880, 50); delay(150);
  }
}

// Animación de lluvia de partículas en sorpresa
void showSurpriseAnimation() {
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    drawScaledBitmap(48, 20, pet_sorprendido2, PET_W, PET_H, WHITE, PET_SCALE);
    // Líneas de exclamación alrededor
    int offsets[] = {0, 4, -4};
    int ox = offsets[i];
    display.drawLine(15+ox, 5,  15+ox, 20, WHITE);
    display.drawPixel(15+ox, 23, WHITE);
    display.drawLine(113+ox, 5, 113+ox, 20, WHITE);
    display.drawPixel(113+ox, 23, WHITE);
    display.display();
    delay(120);
  }
}

// Pantalla de mensaje entrante con efecto de tipo
void showMessageScreen(String text) {
  // Header
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Nuevo mensaje :)");
  display.drawLine(0, 10, 128, 10, WHITE);

  // Tamaño de texto según largo
  int sz = (text.length() > 40) ? 1 : 2;
  display.setTextSize(sz);
  display.setCursor(0, 15);
  display.println(text);
  display.display();
}

// ================================================================
// MANEJO DE TELEGRAM
// ================================================================

void handleNewMessages(int n) {
  for (int i = 0; i < n; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text    = bot.messages[i].text;

    if (chat_id != CHAT_ID) continue;

    // ---- Comandos de emoción ----
    if (text == "/feliz") {
      estadoCaritaActual = FELIZ;
      emotionStartTime   = millis();
      showHappyAnimation();
      playSoundFeliz();
      bot.sendMessage(chat_id, "Carita: ¡Muy feliz! :D", "");

    } else if (text == "/triste") {
      estadoCaritaActual = TRISTE;
      emotionStartTime   = millis();
      playSoundTriste();
      bot.sendMessage(chat_id, "Carita: Un poco triste... :(", "");

    } else if (text == "/enamorado") {
      estadoCaritaActual = ENAMORADO;
      emotionStartTime   = millis();
      showHeartAnimation();
      playSoundEnamorado();
      bot.sendMessage(chat_id, "Carita: Está enamorada <3", "");

    } else if (text == "/dormido") {
      estadoCaritaActual = DORMIDO;
      emotionStartTime   = millis();
      playSoundDormido();
      bot.sendMessage(chat_id, "Carita: Zzz... a dormir.", "");

    } else if (text == "/sorprendido") {
      estadoCaritaActual = SORPRENDIDO;
      emotionStartTime   = millis();
      showSurpriseAnimation();
      playSoundSorprendido();
      bot.sendMessage(chat_id, "Carita: ¡Sorpresa! :O", "");

    } else if (text == "/start") {
      bot.sendMessage(chat_id,
        "LoveBox lista <3\nComandos: /feliz /triste /enamorado /dormido /sorprendido\n"
        "O escribe cualquier mensaje de amor :)", "");

    } else {
      // Mensaje de amor
      playNotificationSound();
      showHeartAnimation();
      mensajeActual = text;
      showMessageScreen(text);
      messageDisplayTime = millis();
      bot.sendMessage(chat_id, "Visto en pantalla <3", "");
    }
  }
}

// ================================================================
// DIBUJO DEL MODO TAMAGOTCHI (RELOJ + MASCOTA)
// ================================================================

// Variables para efectos flotantes
float zzzY    = 45.0;  // posición Y de la Z flotante
float heartFX = 30.0;  // posición de corazoncito flotante
float heartFY = 45.0;

void displayTamagotchiMode() {
  if (!isTimeSynced) {
    displayStatus("Conectado.", "Sincronizando reloj...");
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    displayStatus("ERROR: NTP", "Reintentando...");
    return;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  // ---- CABECERA (Hora + Fecha) ----
  char timeHour[6];
  strftime(timeHour, 6, "%H:%M", &timeinfo);
  display.setTextSize(2);
  display.setCursor(5, 2);
  display.print(timeHour);

  char timeDate[10];
  strftime(timeDate, 10, "%d/%m/%y", &timeinfo);
  display.setTextSize(1);
  display.setCursor(70, 2);
  display.print("Fecha:");
  display.setCursor(70, 12);
  display.print(timeDate);

  display.drawLine(0, 22, 128, 22, WHITE);

  // ---- MASCOTA ----
  const int px = (SCREEN_WIDTH - PET_W * PET_SCALE) / 2;
  const int py = 25;
  const unsigned char* bmp = pet_idle;

  // ---- lógica de animación por estado ----
  switch (estadoCaritaActual) {

    case FELIZ:
      // Alterna entre dos frames de felicidad (brinco)
      bmp = (animFrame % 4 < 2) ? pet_feliz : pet_feliz_b;
      display.setTextColor(WHITE);
      centerText(":D FELIZ!", 57);
      // Estrellitas parpadeantes
      if (animFrame % 2 == 0) {
        display.drawBitmap(5,  30, star_tiny, 8, 8, WHITE);
        display.drawBitmap(115,30, star_tiny, 8, 8, WHITE);
      }
      break;

    case TRISTE:
      // Alterna entre dos frames (con lágrima que aparece)
      bmp = (animFrame % 6 < 3) ? pet_triste : pet_triste_l;
      centerText(":( triste...", 57);
      // Lágrima que cae
      {
        int drop = (animFrame % 6) * 4;
        display.drawPixel(44, 42 + drop, WHITE);
        display.drawPixel(84, 42 + drop, WHITE);
      }
      break;

    case ENAMORADO:
      // Guiño alternado con ojos de corazón
      bmp = (animFrame % 6 < 3) ? pet_enamorado : pet_enamorado_w;
      centerText("<3 enamorada <3", 57);
      // Corazoncito flotante
      heartFY -= 0.4;
      if (heartFY < 22) heartFY = 50.0;
      display.drawBitmap((int)heartFX, (int)heartFY, heart_tiny, 8, 8, WHITE);
      // Segundo corazón en el otro lado
      {
        float hx2 = SCREEN_WIDTH - 20 - heartFX;
        float hy2 = 72 - heartFY;
        if (hy2 > 22 && hy2 < 55)
          display.drawBitmap((int)hx2, (int)hy2, heart_tiny, 8, 8, WHITE);
      }
      break;

    case DORMIDO:
      bmp = pet_dormido;
      centerText("Zzz...", 57);
      // Letra Z flotante
      zzzY -= 0.5;
      if (zzzY < 22) zzzY = 50.0;
      display.setTextSize(1);
      display.setCursor(85, (int)zzzY);
      display.print("z");
      display.setTextSize(2);
      display.setCursor(100, (int)zzzY - 8);
      display.print("Z");
      break;

    case SORPRENDIDO:
      bmp = (animFrame % 4 < 2) ? pet_sorprendido : pet_sorprendido2;
      centerText("!Sorpresa!", 57);
      // Líneas de impacto
      if (animFrame % 2 == 0) {
        display.drawLine(5, 25,  18, 22, WHITE);
        display.drawLine(5, 55,  18, 52, WHITE);
        display.drawLine(123,25, 110, 22, WHITE);
        display.drawLine(123,55, 110, 52, WHITE);
      }
      break;

    case IDLE:
    default:
      // Parpadeo natural: 1 frame de blink cada ~3 segundos
      // animFrame cicla cada ANIMATION_DELAY ms → 200ms*40 = 8s por ciclo completo
      bmp = ((animFrame % 40) < 2) ? pet_blink : pet_idle;
      centerText(">Pulsa el boton", 57);
      break;
  }

  drawScaledBitmap(px, py, bmp, PET_W, PET_H, WHITE, PET_SCALE);
  display.display();
}

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("Error al iniciar SSD1306"));
    while (true);
  }

  displayStatus("Iniciando WiFi...");
  client.setInsecure();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    displayStatus("Conectando WiFi", "Intento: " + String(retries));
    delay(500);
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    displayStatus("WiFi OK!", "Hora Mexico...");
    tone(BUZZER_PIN, 1000, 200);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    int sync_retries = 0;
    while (!getLocalTime(&timeinfo) && sync_retries < 15) {
      displayStatus("WiFi OK", "Sincronizando..." + String(sync_retries));
      delay(1000);
      sync_retries++;
    }

    if (sync_retries < 15) {
      isTimeSynced = true;
      displayStatus("TODO LISTO!", "Disfruta :)");
      // Melodía de bienvenida
      int bienvenida[] = {523, 659, 784, 1047};
      for (int i = 0; i < 4; i++) {
        tone(BUZZER_PIN, bienvenida[i], 100); delay(120);
      }
      noTone(BUZZER_PIN);
      delay(1500);
    }
  } else {
    displayStatus("Error WiFi", "Reinicia");
  }
}

// ================================================================
// LOOP
// ================================================================
void loop() {

  // 1. Auto-retorno a IDLE después de EMOTION_DURATION
  if (estadoCaritaActual != IDLE && emotionStartTime > 0) {
    if (millis() - emotionStartTime > EMOTION_DURATION) {
      estadoCaritaActual = IDLE;
      emotionStartTime   = 0;
      zzzY    = 45.0;
      heartFY = 45.0;
    }
  }

  // 2. Botón físico
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      bot.sendMessage(CHAT_ID, "¡Ella presionó el botón! Te piensa ❤️", "");
      displayStatus("Enviado...", "Te piensa <3");
      playSoundBoton();
      delay(2000);
      messageDisplayTime = 0;
      mensajeActual = "";
    }
  }

  // 3. Telegram
  if (millis() > lastTimeBotRan + BOT_DELAY) {
    int n = bot.getUpdates(bot.last_message_received + 1);
    while (n) {
      handleNewMessages(n);
      n = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  // 4. Control de pantalla
  if (messageDisplayTime > 0 && (millis() - messageDisplayTime < MESSAGE_VIEW_DURATION)) {
    // Mensaje activo: no sobreescribir pantalla
  } else {
    if (messageDisplayTime > 0) {
      messageDisplayTime = 0;
      mensajeActual      = "";
    }

    if (millis() > lastTimeAnimation + ANIMATION_DELAY) {
      animFrame++;
      displayTamagotchiMode();
      lastTimeAnimation = millis();
    }
  }
}

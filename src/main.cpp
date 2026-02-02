#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <esp_heap_caps.h>

TFT_eSPI tft = TFT_eSPI();

// --- ADAPTED FOR 320x240 ---
#define DISPLAY_WIDTH 320 
#define DISPLAY_HEIGHT 240

// WiFi credentials
const char* ssid = "iphone";
const char* password = "123456789";

WiFiServer server(8090);
WiFiClient client;

// Protocol constants
const uint8_t MAGIC[4] = {'P', 'X', 'U', 'P'};
const uint8_t PROTO_VERSION = 0x02;
const size_t HEADER_SIZE = 11;
const uint8_t MAGIC_RUN[4] = {'P', 'X', 'U', 'R'};
const uint8_t RUN_VERSION = 0x01;
const size_t RUN_HEADER_SIZE = 11;

struct PixelUpdate {
  uint16_t x;
  uint16_t y;
  uint16_t len; // Changed to 16-bit to support > 255
  uint16_t color;
};

PixelUpdate* updateBuffer = nullptr;
uint32_t bufferCapacity = 0;

// Dynamic Buffer Logic
bool ensureUpdateBuffer(uint32_t needed) {
  if (needed <= bufferCapacity && updateBuffer != nullptr) return true;
  PixelUpdate* tmp = (PixelUpdate*)ps_malloc(needed * sizeof(PixelUpdate));
  if (!tmp) tmp = (PixelUpdate*)malloc(needed * sizeof(PixelUpdate));
  if (!tmp) return false;
  if (updateBuffer) free(updateBuffer);
  updateBuffer = tmp;
  bufferCapacity = needed;
  return true;
}

bool readExactly(WiFiClient& c, uint8_t* dst, size_t len) {
  size_t got = 0;
  while (got < len && c.connected()) {
    int chunk = c.read(dst + got, len - got);
    if (chunk > 0) got += chunk;
    else delay(1);
  }
  return got == len;
}

void showWaitingScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 20); tft.setTextSize(2);
  tft.println("WIFI PIXEL RX");
  tft.setCursor(10, 50); tft.setTextSize(1);
  tft.println("IP Address:");
  tft.setCursor(10, 70); tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.invertDisplay(true);
  tft.setRotation(1); // Landscape 320x240
  tft.fillScreen(TFT_BLACK);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }

  showWaitingScreen();
  server.begin();
  server.setNoDelay(true);
}

void loop() {
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      client.setNoDelay(true);
      tft.fillScreen(TFT_BLACK);
    }
  }

  if (client && client.connected() && client.available() >= 11) {
    uint8_t magicBuf[4];
    if (readExactly(client, magicBuf, 4)) {
      bool isRun = (memcmp(magicBuf, MAGIC_RUN, 4) == 0);
      bool isPixel = (memcmp(magicBuf, MAGIC, 4) == 0);

      if (isPixel || isRun) {
        uint8_t rest[7]; // version(1), frame_id(4), count(2)
        readExactly(client, rest, 7);
        uint16_t count = rest[5] | (rest[6] << 8);

        if (count > 0 && ensureUpdateBuffer(count)) {
          tft.startWrite();
          for (uint16_t i = 0; i < count; i++) {
            if (isPixel) {
              // Read 6 bytes: X(2), Y(2), Color(2)
              uint8_t entry[6];
              readExactly(client, entry, 6);
              uint16_t x = entry[0] | (entry[1] << 8);
              uint16_t y = entry[2] | (entry[3] << 8);
              uint16_t color = entry[4] | (entry[5] << 8);
              tft.drawPixel(x, y, color);
            } else {
              // Read 8 bytes: Y(2), X(2), Len(2), Color(2)
              uint8_t entry[8];
              readExactly(client, entry, 8);
              uint16_t y = entry[0] | (entry[1] << 8);
              uint16_t x = entry[2] | (entry[3] << 8);
              uint16_t len = entry[4] | (entry[5] << 8); // Now handles > 255
              uint16_t color = entry[6] | (entry[7] << 8);
              
              tft.setAddrWindow(x, y, len, 1);
              tft.writeColor(color, len);
            }
          }
          tft.endWrite();
        }
      }
    }
  }
  delay(1);
}
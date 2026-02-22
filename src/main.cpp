#include <Arduino.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <TFT_eSPI.h>

// --- Configuration ---
const char* ssid = "iphone";
const char* password = "123456789";
const int websocket_port = 81;

// --- Global Objects ---
WebSocketsServer webSocket = WebSocketsServer(websocket_port);
TFT_eSPI tft = TFT_eSPI();

#define JPEG_BUFFER_SIZE (50 * 1024)
uint8_t* jpeg_buffer = nullptr;
uint32_t jpeg_buffer_pos = 0;
uint32_t expected_jpeg_size = 0;

// --- Function Prototypes ---
// In C++, functions must be declared before they are called.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

// --- Implementation ---

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Connection lost!\n", num);
            break;

        case WStype_TEXT: {
            String text = String((char*)payload);
            if (text.startsWith("JPEG_FRAME_SIZE:")) {
                expected_jpeg_size = text.substring(16).toInt();
                jpeg_buffer_pos = 0;
                
                if (expected_jpeg_size > JPEG_BUFFER_SIZE) {
                    Serial.printf("Error: JPEG size (%d) exceeds buffer (%d)!\n", expected_jpeg_size, JPEG_BUFFER_SIZE);
                    expected_jpeg_size = 0;
                }
            }
            break;
        }

        case WStype_BIN:
            if (expected_jpeg_size > 0 && (jpeg_buffer_pos + length) <= expected_jpeg_size) {
                memcpy(jpeg_buffer + jpeg_buffer_pos, payload, length);
                jpeg_buffer_pos += length;
            }

            if (expected_jpeg_size > 0 && jpeg_buffer_pos >= expected_jpeg_size) {
                TJpgDec.drawJpg(0, 0, jpeg_buffer, expected_jpeg_size);
                expected_jpeg_size = 0;
            }
            break;
            
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);

    // Use ps_malloc for ESP32 with PSRAM, otherwise use standard malloc
    jpeg_buffer = (uint8_t*)malloc(JPEG_BUFFER_SIZE);
    if (jpeg_buffer == nullptr) {
        Serial.println("Error: Could not allocate JPEG buffer!");
        while (1) delay(10);
    }

    tft.init();
    tft.setRotation(0); 
    tft.fillScreen(TFT_BLACK);

    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);

    tft.setCursor(10, 10);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.println("Waiting Wi-Fi...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Connection Success!");
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();
}
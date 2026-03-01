#include <Arduino.h>
#include <EasyButton.h>

// Pins 32, 33, 25
EasyButton btnLeft(25);
EasyButton btnRight(33);
EasyButton btnZ(32);

// State trackers to prevent "spamming" the Serial port
bool leftHeld = false;
bool rightHeld = false;
bool zHeld = false;

void setup() {
    Serial.begin(115200);
    btnLeft.begin();
    btnRight.begin();
    btnZ.begin();
    Serial.println("READY");
}

void loop() {
    btnLeft.read();
    btnRight.read();
    btnZ.read();

    // --- LEFT (GPIO 25) ---
    if (btnLeft.isPressed() && !leftHeld) {
        Serial.println("LEFT_DN");
        leftHeld = true;
    } else if (!btnLeft.isPressed() && leftHeld) {
        Serial.println("LEFT_UP");
        leftHeld = false;
    }

    // --- RIGHT (GPIO 33) ---
    if (btnRight.isPressed() && !rightHeld) {
        Serial.println("RIGHT_DN");
        rightHeld = true;
    } else if (!btnRight.isPressed() && rightHeld) {
        Serial.println("RIGHT_UP");
        rightHeld = false;
    }

    // --- Z KEY (GPIO 32) ---
    if (btnZ.isPressed() && !zHeld) {
        Serial.println("Z_DN");
        zHeld = true;
    } else if (!btnZ.isPressed() && zHeld) {
        Serial.println("Z_UP");
        zHeld = false;
    }
}
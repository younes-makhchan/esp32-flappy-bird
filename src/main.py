import serial
import pydirectinput
import time

# Open Serial with 0 timeout for instant response
ser = serial.Serial('COM7', 115200, timeout=0) 

# Memory: True = Finger is ON button, False = Finger is OFF
is_left = False
is_right = False
is_z = False

print("Mario Controller: HOLD mode active.")

while True:
    # 1. Check for updates from ESP32
    if ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            print(f"Received: {line}")
            # Update States
            if line == "LEFT_DN":   is_left = True
            elif line == "LEFT_UP":  is_left = False; pydirectinput.keyUp('left')
            
            if line == "RIGHT_DN":  is_right = True
            elif line == "RIGHT_UP": is_right = False; pydirectinput.keyUp('right')
            
            if line == "Z_DN":      is_z = True
            elif line == "Z_UP":     is_z = False; pydirectinput.keyUp('z')
        except:
            pass

    # 2. THE ACTION: If state is True, keep the key DOWN
    # This runs every loop (very fast) so the game never "loses" the press
    if is_left:  pydirectinput.keyDown('left')
    if is_right: pydirectinput.keyDown('right')
    if is_z:     pydirectinput.keyDown('z')

    # 3. Frequency: 0.01 is a good balance for Mario
    time.sleep(0.01)
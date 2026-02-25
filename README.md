# LED Tetris - ESP32 LED Matrix Game

A Tetris game implementation for ESP32 with MAX7219/MAX7221 LED matrix display.

## Project Overview
This project is a Tetris game that runs on an ESP32 microcontroller and displays on a 4-module LED matrix using the MAX72XX driver. The game includes:
- Classic Tetris gameplay with 7 different tetriminos
- Button controls for movement and rotation
- Score tracking and line clearing
- Animated piece dropping
- Game over detection

## Hardware Requirements
- ESP32 development board (ESP32-DevKit, ESP32-WROOM, etc.)
- 4x MAX7219/MAX7221 LED matrix modules (8x8 LED each, 32x8 total display)
- 2 push buttons for game controls
- Jumper wires for connections

## Pin Connections
The default pin configuration is:
- **CLK_PIN**: 18 (Clock signal for MAX72XX)
- **DATA_PIN**: 23 (Data signal for MAX72XX)
- **CS_PIN**: 5 (Chip select for MAX72XX)
- **BUTTON_A**: 32 (Left/rotate controls)
- **BUTTON_B**: 33 (Right/drop controls)

## Software Requirements
- PlatformIO IDE or CLI
- Arduino framework
- MD_MAX72XX library (v3.5.1 or higher)
- EasyButton library (v2.0.3 or higher)

## Setup Instructions

### 1. Install PlatformIO
If you haven't installed PlatformIO yet:
- For VS Code: Install the PlatformIO IDE extension
- For CLI: Install PlatformIO Core using `pip install platformio`

### 2. Clone/Download the Project
Clone this repository or download the project files to your local machine.

### 3. Install Dependencies
The project uses PlatformIO's library management system. Dependencies are automatically installed when you build the project.

### 4. Fix EasyButton Library Issue
The EasyButton library has a known issue where `filters.h` doesn't exist. Follow these steps to resolve it:

1. After installing dependencies, locate the EasyButton library in your PlatformIO libraries folder
2. Open the `EasyButton` library
3. Follow the instruction from the GitHub issue: https://github.com/evert-arias/EasyButton/issues/88

### 4. Configure the Project
The main configuration is in `platformio.ini`:
- Board: ESP32-DevKit (can be changed to your specific ESP32 board)
- Framework: Arduino
- Libraries: MD_MAX72XX and EasyButton

### 5. Build and Upload
- Open the project in PlatformIO
- Select your ESP32 board from the bottom status bar
- Click the "Build" button (checkmark icon) to compile
- Click the "Upload" button (right arrow icon) to flash to your ESP32

### 6. Monitor Serial Output
- Open the Serial Monitor (bug icon or Ctrl+Alt+M)
- Set baud rate to 115200
- You should see the game start and display messages

## How to Play

### Controls
- **Button A (short press)**: Move piece left
- **Button A (long press)**: Rotate piece clockwise
- **Button B (short press)**: Move piece right
- **Button B (long press)**: Drop piece instantly

### Game Mechanics
- Pieces fall automatically at increasing speeds as you clear lines
- Complete horizontal lines to clear them and earn points
- The game ends when pieces stack to the top of the display
- Score is based on piece types and rotations

## Project Structure
```
LEDSYNC/
├── platformio.ini          # PlatformIO configuration
├── src/
│   ├── main.cpp           # Main game loop and hardware setup
│   ├── tetris.cpp         # Tetris game engine implementation
│   └── tetris.h           # Tetris game engine header
├── include/
│   └── README            # Include directory documentation
├── lib/
│   └── README            # Library directory documentation
├── test/
│   └── README            # Test directory documentation
└── images/
    └── screen_shot.jpg    # Project screenshot
```

## Customization Options
You can modify the game behavior by changing constants in `main.cpp`:
- `HARDWARE_TYPE`: LED matrix type (default: FC16_HW)
- `MAX_DEVICES`: Number of LED matrix modules (default: 4)
- `TEXT_SCROLL_DELAY`: Speed of text scrolling
- `BTN_LONG_DURATION`: Duration for long button press detection

## Troubleshooting
- **Build errors**: Ensure all libraries are installed and up to date
- **Upload failures**: Check USB connection and correct board selection
- **Display issues**: Verify wiring connections and pin assignments
- **Button problems**: Check button wiring and debounce settings

## License
This project is open source. See the LICENSE file for details.

## Contributing
Feel free to submit issues and enhancement requests!
# stm32-temp-controller

![Language](https://img.shields.io/badge/language-C-blue)
![MCU](https://img.shields.io/badge/MCU-STM32F103RET6-green)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-blueviolet)
![License](https://img.shields.io/badge/license-MIT-yellow)

STM32F103 temperature controller with LCD menu, UART tuning, MPU6050 sensor and PWM output.

## Table of Contents
- [Overview](#overview)
- [Hardware](#hardware)
- [Features](#features)
- [Menu Structure](#menu-structure)
- [How to Use](#how-to-use)
- [UART Protocol](#uart-protocol)
- [Pin Connections](#pin-connections)
- [Project Structure](#project-structure)
- [Build and Flash](#build-and-flash)
- [Future Work](#future-work)
- [Authors](#authors)
- [License](#license)

## Overview
This project is a small embedded temperature controller built on an STM32F103 microcontroller. The user navigates a menu on a 16x2 character LCD with three buttons, sets a desired temperature and an offset (by hand or over UART), and the board drives a PWM output proportional to the difference between the received temperature and the desired temperature. Settings are stored in internal Flash, so they survive a power cycle.

## Hardware
| Component | Description |
|-----------|-------------|
| STM32F103RET6 | Main microcontroller (HSI clock, 8 MHz) |
| 16x2 Character LCD | Menu and value display |
| MPU6050 | Temperature sensor over I2C |
| 3 push buttons | Next / Select / Confirm |
| LED (PC8) | Status LED, blinks after saving |
| LED (PC9) | Warning LED for negative offset |
| LED / load on TIM8 CH2 (PC7) | PWM output |

## Features

- LCD menu with three entries: **Desired**, **offset**, **Tune Temp**
- Set the desired temperature and the offset by buttons (**Hand Tune**) or via **UART**
- Settings are saved in the internal Flash and restored after reset
  - Defaults: temperature = 25, offset = 2
- Live temperature display from the MPU6050
- PWM duty cycle proportional to the error: `duty = min(|error| x 4, 100)` percent
- Blinking LED confirms that a value was saved
- Automatic return to the main menu after 5 seconds of inactivity
- UART timeout: returns to the main menu after 10 seconds without data

## Menu Structure
```
Main Menu
├── Desired
│   ├── Hand Tune   (edit desired temperature with buttons)
│   └── UART        (receive temperature over UART)
├── offset
│   ├── Hand Tune   (edit offset with buttons)
│   └── UART
└── Tune Temp       (show live MPU6050 temperature for 5 seconds)
```

## How to Use
| Button | In menus | In Hand Tune mode |
|--------|----------|-------------------|
| **Next** (PB11) | Move to the next item | Increase value (+1) |
| **Select** (PB12) | Enter the selected item | Decrease value (-1) |
| **Confirm** (PB13) | - | Save the value to Flash |

1. Power on the board. The main menu appears on the LCD.
2. Press **Next** to move the `>` cursor, press **Select** to enter an item.
3. In **Hand Tune**, use **Next** (+) and **Select** (-) to change the value.
4. Press **Confirm** to save. The LED on PC8 blinks and the board returns to the main menu.
5. In **UART** mode, send a packet (see below). The LCD shows the received temperature.

## UART Protocol
Settings: **115200 baud, 8 data bits, no parity, 1 stop bit (8N1)**

Packet format (4 bytes):

| Byte | Value | Meaning |
|------|-------|---------|
| 1 | `0xEE` | Start byte |
| 2 | High | Temperature high byte |
| 3 | Low | Temperature low byte |
| 4 | `0xAA` | End byte |

Board responses:

| Response | Meaning |
|----------|---------|
| `ready` | Start byte received |
| `OK` | Valid packet received |
| `ERR` | End byte was wrong |

Example: to send a temperature of 30 (`0x001E`), send the bytes
`EE 00 1E AA`.

## Pin Connections
| Function | Pin |
|----------|-----|
| Next button | PB11 |
| Select button | PB12 |
| Confirm switch | PB13 |
| I2C1 SCL | PB6 |
| I2C1 SDA | PB7 |
| UART1 TX / RX | PA9 / PA10 |
| Status LED | PC8 |
| Warning LED | PC9 |
| PWM output (TIM8 CH2) | PC7 |
| LCD data / control | PC0 - PC6 |

## Project Structure
```
stm32-temp-controller/
├── Core/
│   ├── Inc/            Header files (main.h, lcd.h, ...)
│   └── Src/            Source files (main.c, lcd.c, ...)
├── Drivers/            STM32 HAL and CMSIS drivers
├── Phase1.ioc          STM32CubeMX configuration
├── STM32F103RETX_FLASH.ld   Linker script
└── README.md
```

## Build and Flash
**Requirements:** STM32CubeIDE and an ST-Link programmer.

1. Clone the repository:
   ```bash
   git clone https://github.com/Heliakhalilirad/stm32-temp-controller.git
   ```
2. In STM32CubeIDE: **File → Import → Existing Projects into Workspace**, and select the cloned folder.
3. Build with **Project → Build All** (Ctrl + B).
4. Connect the board with ST-Link and press **Run** to flash.

## Future Work
- Use the MPU6050 temperature as the control input instead of the UART value
- Store the offset as a signed value in Flash
- Add a proper PID controller for the PWM output
- Add button debouncing in hardware or software

## Authors
- **Helia Khalilirad** - [@Heliakhalilirad](https://github.com/Heliakhalilirad)
- **Negin Safari** - [@negin_safari_ee](https://github.com/negin_safari_ee)

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

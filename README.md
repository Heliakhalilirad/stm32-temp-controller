# stm32-temp-controller

STM32F103 temperature controller with LCD menu, UART tuning, MPU6050 sensor and PWM output.

## Hardware
- STM32F103 microcontroller
- Character LCD (16x2)
- MPU6050 sensor (I2C) for temperature reading
- 3 buttons: Next / Select / Confirm
- LEDs: status LED (PC8), offset warning LED (PC9), PWM output (TIM8 CH2)

## Features
- LCD menu with three options: **Desired**, **offset**, **Tune Temp**
- Set desired temperature and offset by buttons (Hand Tune) or via UART
- Settings are saved in internal Flash and restored after reset
- Live temperature display from MPU6050
- PWM output proportional to the difference between received and desired temperature
- Automatic return to the main menu after a timeout

## UART Protocol
Baud rate: 115200, 8N1

| Byte | Meaning |
|------|---------|
| 0xEE | Start |
| High | Temperature high byte |
| Low  | Temperature low byte |
| 0xAA | End |

The board replies with `ready`, `OK` or `ERR`.

## Pin Connections
| Function | Pin |
|----------|-----|
| Next button | PB11 |
| Select button | PB12 |
| Confirm switch | PB13 |
| I2C SCL / SDA | PB6 / PB7 |
| UART1 | PA9 / PA10 |
| PWM output | TIM8 CH2 |

## Build
1. Open the project in STM32CubeIDE.
2. Build and flash to the board.

## Authors
- Helia khalilirad
- Negin safari

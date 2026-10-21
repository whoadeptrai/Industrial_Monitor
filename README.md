# Industrial Monitor: IoT Fire & Gas Incident Monitoring and Response System

A dual-MCU incident monitoring system. An **STM32F103** handles all safety-critical logic locally and in real time, while an **ESP8266** acts as a WiFi gateway that pushes data to a **Blynk** dashboard. If the network or WiFi goes down, fire and gas protection keeps working.

Team project (Group 6) from the Machine Learning & IoT Lab, Faculty of Electrical & Electronics Engineering, Ho Chi Minh City University of Technology (HCMUT).

## Why dual-MCU

| Approach | Strength | Weakness |
|----------|----------|----------|
| Traditional local alarm | Very reliable, works offline | No remote visibility, no environmental data for early warning |
| Single-MCU IoT device | Remote monitoring from phone/web | A network hang or WiFi loss can disable the whole alarm system |
| **This project (STM32 + ESP8266)** | Offline automation and remote monitoring | Two firmwares to maintain |

The STM32 never depends on the network. The ESP8266 only relays data, so networking work is fully offloaded from the core controller.

## Features

- **Interrupt-driven detection:** flame and gas sensors are digital inputs wired to hardware interrupts, so the system reacts the moment a threshold is crossed instead of waiting for a polling loop
- **Automatic response:** a relay switches a pump/fan (220 V load) and a servo closes a safety valve
- **Latching alarm:** once triggered, the alarm state stays locked until it is handled
- **Background monitoring:** temperature and humidity read from a DHT11 (1-Wire)
- **Remote monitoring:** environment data and Fire/Gas alarm flags are sent to Blynk through the ESP8266
- **Maintenance mode:** a technician can test the relay and servo with an analog joystick or the app, only when the system is safe

## Hardware

| Block | Parts |
|-------|-------|
| Main controller | STM32F103 |
| IoT gateway | ESP8266 |
| Hazard sensors | MKE-S04 IR flame sensor, MQ-2 gas/smoke sensor (digital output, interrupt) |
| Environment sensor | DHT11 temperature and humidity |
| Actuators | Isolated relay (pump/fan), servo (safety valve, PWM) |
| Manual controls | Analog joystick (ADC, 0-4095), push buttons |

**Pin mapping:** see `Industrial_Monitor.ioc` (open in STM32CubeMX). <!-- TODO: add a table: part -> pin -> peripheral -->

## System architecture

```
 Flame / MQ-2 ──(EXTI interrupt)──┐
 DHT11 (1-Wire) ──────────────────┤
 Joystick (ADC) / buttons ────────┤
                                  ▼
                        STM32F103 (state machine)
                          │              │
              Relay / Servo (PWM)        UART 115200
                                         │
                                         ▼
                                  ESP8266 gateway ──WiFi──► Blynk app
```

The firmware is organized around the Active Object pattern (`micro_ao`): each task owns its state and handles one event at a time, so tasks do not block each other.

## State machine

| State | Behavior |
|-------|----------|
| **IDLE** | Normal operation. Waits for sensor events. Relay and servo stay at default positions. Reports connection status to the app. |
| **ALARM** | Entered from a sensor interrupt. Latches, closes the valve (servo), turns on the pump (relay), and sends a red alert to the app. |
| **MANUAL** | Maintenance mode. Only available from IDLE. Joystick or app controls the relay and servo for testing. Disabled automatically if a fire alarm occurs. |

## Communication

STM32 and ESP8266 talk over UART (115200 baud) using an asynchronous, command-line style protocol. The ESP8266 uploads environment data and the Fire/Gas flags to Blynk. <!-- TODO: add 1-2 example UART messages -->

## Repository layout

| Path | Content |
|------|---------|
| `Core/` | CubeMX-generated startup code, peripheral init, `main` |
| `Drivers/` | STM32 HAL and CMSIS |
| `app/` | Application code |
| `micro_ao/` | Lightweight Active Object framework |
| `cmake/` | Toolchain and build helpers |
| `scripts/` | Helper scripts |
| `docs/` | Project report and documentation |
| `Industrial_Monitor.ioc` | STM32CubeMX project |
| `CMakeLists.txt`, `CMakePresets.json` | Build configuration |


## Build and flash (STM32)

Requirements: `arm-none-eabi-gcc`, CMake, Ninja or Make.

```bash
cmake --list-presets          # see available presets
cmake --preset Debug          # replace Debug with a preset from the list
cmake --build --preset Debug
```

Flash the generated `.elf` with an ST-Link, for example using STM32CubeProgrammer or OpenOCD:

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Debug/Industrial_Monitor.elf verify reset exit"
```

## Usage

1. Wire the hardware as defined in the `.ioc` file and power the system.
2. Configure the ESP8266 with your WiFi and Blynk credentials. Do not commit credentials to the repository.
3. Let the MQ-2 warm up, then test with a safe flame or smoke source.
4. Watch the state change from IDLE to ALARM, the relay and servo respond, and the alert appear in Blynk.



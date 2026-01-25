<p align="center">
<a href="https://www.bilibili.com/video/BV1JnivBpEYX" target="_blank">Demo Video</a>｜
<a href="./README.zh-CN.md">简体中文</a>
</p>

# Gorb Overview

Gorb is a mouse project using ESP32-S3 as the main controller, supporting dual-mode switching between USB and wireless (
BLE),
and featuring hot-swappable replacement for 4 micro-switch buttons.

## Main Features

* Basic mouse functions with two additional side buttons, all supporting hot-swap.
* Driver support for DPI setting and movement macros.
* Up to 1000Hz polling rate.
* Uses user operation interrupt service (ISR) and event-driven design, eliminating polling to reduce CPU consumption.
  Combined with ESP-PM to enter power-saving sleep mode when idle.
* Reports accumulated difference values, ensuring minimal loss of operations.

![Gorb](https://gorb-1301890033.cos.ap-chengdu.myqcloud.com/gorb.jpg)

### Experience

The conclusion is straightforward: it's not as good as my own G Pro, but it's more usable than the office mice at work. 
The main reason is that the resin shell has an unsatisfactory feel (somewhat soft, and the surface is relatively rough after the polishing process). For future reproductions, consider changing the material.

## How to Use

### Project Structure

    gorb/
    |- app/
    |  |- driver/       Driver program
    |  |- mouse/        Mouse program
    |- design/          Design --Deleted, go https://oshwhub.com/no_panic/gorb to get design project
    |- shell/           3D shell
    |- README.md

### Hardware Requirements

Essentially a minimal system board. Considering ease of manual soldering, an ESP32 wireless module is used.
Refer to the Gorb hardware design for details: [Gorb Design](https://oshwhub.com/no_panic/gorb).\
The design includes a PCB with an additional receiver board. If you wish to use the 2.4G mode, you can start by exploring this board. In previous tests, I found that battery power might be unstable when the ESP32's radio frequency (RF) is operating frequently. Therefore, this project does not include a Wi-Fi mode. If you manage to resolve this issue, I would greatly appreciate it if you could submit a Pull Request. Thank you very much!

#### Main Hardware

* Mouse Main Controller: ESP32-S3-WROOM series chip
* Optical Sensor: PixArt PAW3395DM

#### 3D Shell

The 3D shell is from [MatNS](https://www.thingiverse.com/MatNS/designs)
Published on
Thingiverse: [G Pro Wireless - G305 Shell Swap Mod MatNS edit V2](https://www.thingiverse.com/thing:4727172), thanks to
open source.

### Firmware Flashing

The mouse are developed based on the ESP-IDF framework. Before flashing, please install ESP-IDF or
IDE-based plugins first.\
If using the VSCode ESP-IDF plugin, you can follow these steps to flash:

    1.Open ESP-IDF Explorer
    2.Click "ESP-IDF: Select Flash Method" and choose UART
    3.Click "Set Espressif Device Target" and select esp32s3 (mouse)
    4.Go to "SDK Configuration Editor (menuconfig)" and modify Flash and PSRAM configurations based on the specific chip model
    5.Click "Full Clean" to clean previous build files
    6.Click "Build Project" to build the project and wait for completion
    7.Power off the board (if the mouse has a battery inserted, turn off the power switch)
    8.Hold the Boot button on the board, power on for a short time and release, then connect to the computer via USB (for mouse without battery, directly connect USB to power on). If the hardware soldering is fine, clicking "Select Port to Use" should show a new port; select the corresponding port
    9.Click "Flash Project" to flash the project and wait for completion
    10.Power off again, and repower to connect to the computer

### Driver Usage

* The driver is developed in Python, and the release version includes a basic .exe program. Currently, the driver only
  supports mouse connection via USB; it cannot list devices connected via BLE (but does not affect normal use).
* After running the program, input "help" to view detailed usage.

![Gorb Driver](https://gorb-1301890033.cos.ap-chengdu.myqcloud.com/driver.png)

## Contributing

Welcome to contribute to Gorb through the following ways:

* Report bugs or consult via [Issue](https://github.com/UniYUIN/Gorb-ESP32WirelessMouse/issues).
* Submit [Pull Request](https://github.com/UniYUIN/Gorb-ESP32WirelessMouse/pulls) to improve Gorb's code.# Gorb-ESP32WirelessMouse

>If it was helpful to you, please consider giving gorb a star ⭐

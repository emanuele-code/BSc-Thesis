# BSc-Thesis

Bare-metal cooperative user-space threads for Arduino Uno (ATmega328P, 16 MHz), written in C++17 with avr-libc only (no Arduino core).

The project compares two kinds of threads on flash usage, RAM usage and context-switch cost:

- **Stackless**: protothreads, built on macros (`include/protothread.hpp`).
- **Stackful**: each thread has its own stack and the context switch is written in AVR assembly (`src/context_switch.S`).

Work in progress: a common scheduler for both kinds is still to be written.

## Project Structure

```text
.
├── include
│   ├── protothread.hpp
│   ├── thread.hpp
│   ├── timer.hpp
│   ├── scheduler.hpp
│   ├── utilityPin.hpp
│   └── utilitySerial.hpp
├── src
│   ├── context_switch.S
│   ├── main.cc
│   ├── thread.cc
│   ├── timer.cc
│   └── scheduler.cc
├── docs
│   ├── atemega328p.pdf
│   └── stato_progetto.pdf
├── cmake
│   └── avr-g++.cmake
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Requirements

Ubuntu 24.04:

```bash
sudo apt update
sudo apt install build-essential cmake gcc-avr binutils-avr avr-libc avrdude
```

## Build

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/avr-g++.cmake
cmake --build build
```

If you change toolchain or compiler, delete `build/` first. The build produces `build/main.hex` and prints flash/RAM usage.

## Flash

Connect the board (it shows up as `/dev/ttyACM0`) and run:

```bash
avrdude -p atmega328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:build/main.hex:i
```

To read the UART output (9600 baud):

```bash
screen /dev/ttyACM0 9600
```
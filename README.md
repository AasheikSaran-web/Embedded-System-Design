# Embedded System_Design
Assignment  Solutions for Embedded System Design Course in IISC Bangalore


# Embedded Systems Projects and Source Code

This repository contains a variety of C programs developed for embedded systems using the Tiva C (TM4C123GH6PM) microcontroller. Each project showcases different features such as timers, UART communication, ADC, PWM, CAN bus, and interfacing with peripherals.

## Project List

###  CPS Twinning over CAN Bus
**File:** `CPS Twinning over CAN Bus.zip`  
Implements CPS (Cyber-Physical System) Twinning over a CAN bus with RTC time sync, DC motor mirroring, and LCD display.

###  DC Motor Control
**File:** `DC motor control using ADC, Timer and PWM.c`  
Controls a DC motor's speed using ADC input from a potentiometer, with PWM output and timer-based logic.

###  Heap Algorithm
**File:** `Heap_Algorithm.c`  
Implements a heap sort or priority queue algorithm in C.

###  Keypad and Seven Segment Display
**File:** `Keypad and Seven Segment Display.c`  
Interfacing a 4x4 matrix keypad and 7-segment display to take input and display digits or results.

###  Milestone Files
**Files:**  
- `MILESTONE.c`  
- `MILESTONE2.C`  
- `MILESTONE3.c`  
These are the answers for the LAB TEST

###  Peek and Poke
**File:** `PeekandPoke.c`  
Provides functionality to read/write memory locations, often used for debugging or low-level interfacing.

###  RTC and LED Display
**File:** `RTC_LED.c`  
Displays the current time read from an RTC module on an LED or LCD display.

###  Stopwatch with LED
**File:** `Stopwatch Timer and LED control using Interrupts.c`  
Implements a stopwatch timer using interrupts and controls LEDs based on timer state.

###  UART Command Console
**File:** `UART Console.c`  
Implements a UART-based console for issuing commands like `led on red`, `timer set 5`, etc.

###  Tic-Tac-Toe Game
**File:** `tictacktoe.c`  
Implements a text-based tic-tac-toe game in C, possibly for learning input handling or game logic.

---

## Getting Started

To build and run any of these projects, use [Code Composer Studio (CCS)](https://www.ti.com/tool/CCSTUDIO) and a Tiva C LaunchPad board. Ensure the required libraries (like TivaWare) and hardware (e.g., keypad, 7-segment, motor driver) are connected as per each project’s needs.

## License

Open-source for educational and non-commercial use.



# IRIS2-firmware-flight
Firmware for the IRIS2 CPU, that will fly as an instrument on the mission Sunrise III

## Prerequisites
You should install Code Composer Studio (Version 10 or higher preffered) https://www.ti.com/tool/CCSTUDIO

## Programming the MCU
You should have a programmer for MSP430, a launchpad is enough.
In the docs/ folder you will find all the documents with detailed description and datasheets of the hardware involved.

In order to program the MCU you must connect the following lines:
* GND
* SBWTDIO
* SBWTCK

The following lines are not always needed:
* 3V3, Only if you are not providing external power supply
* RDX, Only if you need access to Front Plate (Command line) UART 
* TXD, Only if you need access to Front Plate (Command line) UART

For easy access :)
<div align="center">
<img src="docs/JTAG_connection.png" alt="JTAG connection with Launchpad" width="70%"/>
</div>

## Global Overview of the Board
Some of the files that you MUST read before trying to program the MCU:
* Pinouts in detail: [docs/IRIS2_pinouts.xlsx](docs/IRIS2_pinouts.xlsx)
* Schematics of the board: [docs/IRIS2_PCB_CPU_v1.3.pdf](docs/IRIS2_PCB_CPU_v1.3.pdf)

<div align="center">
<img src="docs/IRIS2_CPU.png" alt="IRIS2 CPU Block Diagram" width="70%"/>
</div>

## Command line instructions
TBC

## Command line examples
TBC

## Authors
* Aitor Conde <aitorconde@gmail.com>
* Ramón García <ramongarciaalarcia@gmail.com>


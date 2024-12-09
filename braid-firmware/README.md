# BRAID firmware
This repository contains code for the BRAID hardware devices, structured as follows:

| Folder        | Description                                            |
| ------------- | ------------------------------------------------------ |
| `assets`      | Images and other data for the project's documentation  |
| `master-mcu`  | Firmware for the master board, based on ESP32          |
| `scripts`     | Various script used to provision the device            |
| `sensors-mcu` | Firmware for the sensors board, based on STM32L0       |
| `shared`      | Shared C code used in both master and sensors firmware |

For board-specific information and provisioning instructions, refer to the the READMEs contained in the various folders.

For notes and experimentation logs of working with GSM modems, see [GSM_MODEMS.md](GSM_MODEMS.md).
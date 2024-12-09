# Sensors MCU
This directory hosts code code for the sensor-facing MCU, based on STM32 L0.

### Building
Use `platformio` through `pio run` or plain `cmake` and `make`.

### Flashing
Using `openocd` installed by PlatformIO, the command is the following:

```sh
~/.platformio/packages/tool-openocd/bin/openocd -d2 -s ~/.platformio/packages/tool-openocd/openocd/scripts -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32l0.cfg -c "program {.pio/build/nucleo_l031k6/firmware.elf}  verify reset; shutdown;"
```

Flashing EEPROM data can be done with the `stlink`:

```sh
python ../scripts/generate_factory_data.py -o factory.bin --key $KEY --id $ID --type $TYPE && st-flash --flash=1m write factory.bin 0x08080000
```

### Updating chip configuration with STM32CubeMX
> [!CAUTION]
> STM32CubeMX WILL DELETE code which is not contained inside the special comments `/* USER CODE BEGIN */` and `/* USER CODE END */`

This project is fully compatible with STM32CubeMX code generation. The workflow is as follows:
1. Download the STM32CubeMX installer from the official website (a user account is required) or from Sharepoint and install it
2. Download the [`stm32pio`](https://github.com/ussserrr/stm32pio) utility with `pipx install stm32pio`
3. Open the `sensors-mcu.ioc` with STM32CubeMX and update the configuration, then click on "Generate Code"
4. Run `stm32pio new -b nucleo_l031k6 --with-build` to regenerate the code
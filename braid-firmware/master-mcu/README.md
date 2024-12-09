# Master MCU
This directory hosts code code for the master MCU, based on ESP32.

### Flashing
After building, the firmware can be flashed with

```sh
"$HOME/.platformio/penv/bin/python" "$HOME/.platformio/packages/tool-esptoolpy/esptool.py" --chip esp32s3 --port $PORT --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0 .pio/build/t-sim7000/bootloader.bin 0x8000 .pio/build/t-sim7000/partitions.bin 0x6d0000 .pio/build/t-sim7000/ota_data_initial.bin 0x10000 .pio/build/t-sim7000/firmware.bin
```

Then factory data must be generated with:

```sh
python ../scripts/generate_factory_data.py master -o factory.bin <parameters>
```

Use `python ../scripts/generate_factory_data.py -h` for more information on the required parameters.

> [!CAUTION]
>
> The firmware **does not support DNS queries!** As such, the MQTT broker URL must be supplied as a numeric IPv4.

The data must then be flashed with:

```sh
# This also programs the example CA certificate
python ../scripts/send_factory_data.py -p $PORT factory.bin ../scripts/esp_cryptoauth_utility/certs/ca.crt
```
See `python ../scripts/send_factory_data.py -h` for more information.

### Provisioning certificates
To provision the ATECC608 connected to the main MCU the `secure_cert_mfg.py` script can be used

```sh
python ../scripts/esp_cryptoauth_utility/secure_cert_mfg.py --signer-cert certs/signer.crt --signer-cert-private-key certs/signer.key --port $PORT --target_chip esp32 --no-wait
```

> [!NOTE]
>
> For the SIM7000G devkit, `<sda>` is 21 and `<scl>` is 22

For more information, visit the `esp-cryptoauthlib` [project homepage](https://github.com/espressif/esp-cryptoauthlib/tree/master/esp_cryptoauth_utility).

### Shadow payload

| Key     | CBOR Data Type | Description                                                  |
| ------- | -------------- | ------------------------------------------------------------ |
| FW_VER  | string         | Firmware version string                                      |
| DELAY   | uint64         | Delay in seconds between network connections for data report |
| TEMP    | int64          | Measured temperature                                         |
| HUMID   | int64          | Measured humidity                                            |
| ACCEL_X | float          | Acceleration in the X-axis                                   |
| ACCEL_Y | float          | Acceleration in the Y-axis                                   |
| ACCEL_Z | float          | Acceleration in the Z-axis                                   |
| DIR     | float          | Direction of movement in degrees, with 0 being north         |
| LAT     | float          | Latitude                                                     |
| LON     | float          | Longitude                                                    |
| HSPEED  | float          | Speed                                                        |
| ALT     | float          | Altitude                                                     |
| H_ACC   | float          | Horizontal dilution of precision                             |

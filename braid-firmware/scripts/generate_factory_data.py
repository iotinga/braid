import argparse
import functools
import struct
from dataclasses import dataclass
from pathlib import Path

MAC_ADDR_MAGIC_STR = "__MAC_ADDR__"


@dataclass
class CommonArgs:
    key: str
    output: Path


@dataclass
class MasterArgs(CommonArgs):
    id: str | None
    mqtt_uri: str
    sim_apn: str
    sim_pin: str | None


@dataclass
class SensorArgs(CommonArgs):
    id: int
    type: int


def len_lte_128(s: str):
    if len(s) == 128:
        return s
    raise argparse.ArgumentTypeError("Key must be 128 characters long")


def sensor_command(args: SensorArgs):
    with args.output.open("wb+") as f:
        f.write(args.key.encode("ascii"))
        # id and type are uint32_t
        f.write(struct.pack("<II", args.id, args.type))
        # anti tamper is set to 0 (uninitialized)
        f.write(struct.pack("<BB", 0, 0))


def master_command(args: MasterArgs):
    with args.output.open("wb+") as f:
        f.write(args.key.encode("ascii"))
        # Device ID is a string stored in a 32 byte buffer
        f.write(struct.pack("<32s", args.id.encode("ascii")))
        # MQTT uri is a string stored in 256 bytes
        f.write(struct.pack("<256s", args.mqtt_uri.encode("ascii")))
        # SIM APN is a string stored in 32 bytes
        f.write(struct.pack("<32s", args.sim_apn.encode("ascii")))
        # Set `simPinRequired` to `true` if the pin was set via CLI
        f.write(struct.pack("<?", args.sim_pin is not None))
        # SIM APN is a string stored in 8 bytes
        sim_pin = "" if args.sim_pin is None else args.sim_pin
        f.write(struct.pack("<8s", sim_pin.encode("ascii")))


def add_common_args(parser):
    parser.add_argument(
        "--output",
        "-o",
        help="the output file path",
        default=Path.cwd() / "factory.bin",
        type=Path,
        required=True,
    )
    parser.add_argument(
        "--key",
        help="the HMAC key for inter-MCU message signing",
        type=len_lte_128,
        required=True,
    )


def main():
    parser = argparse.ArgumentParser(description="generates a .bin file containing EEPROM data for the MCUs")

    subparsers = parser.add_subparsers(title="subcommands")

    # Sensor subcommand
    parser_sensor = subparsers.add_parser("sensor", help="Generate data for the sensors MCU")
    parser_sensor.add_argument(
        "--id",
        help="the sensor ID, for example 0x00000001",
        type=functools.partial(int, base=0),
        required=True,
    )
    parser_sensor.add_argument(
        "--type",
        help="the sensor type bit mask",
        type=functools.partial(int, base=0),
        required=True,
    )
    parser_sensor.set_defaults(handler=sensor_command)
    add_common_args(parser_sensor)

    # Master subcommand
    parser_master = subparsers.add_parser("master", help="Generate data for the master MCU")
    parser_master.add_argument(
        "--id",
        help="the master device ID, for example a MAC address. If not specified, the ESP32's own MAC address will be used",
        type=str,
        required=False,
        default=MAC_ADDR_MAGIC_STR,
    )
    parser_master.add_argument(
        "--mqtt-uri",
        "-m",
        dest="mqtt_uri",
        help="the full URI of the MQTT broker (for example 'mqtt://test.mosquitto.org:1883')",
        type=str,
        required=True,
    )
    parser_master.add_argument(
        "--sim-apn",
        "-a",
        dest="sim_apn",
        help="APN of the SIM's provider",
        type=str,
        required=True,
    )
    parser_master.add_argument(
        "--sim-pin",
        "-p",
        dest="sim_pin",
        help="PIN used to unlock the SIM",
        type=str,
    )
    parser_master.set_defaults(handler=master_command)
    add_common_args(parser_master)

    args = parser.parse_args()
    args.handler(args)


if __name__ == "__main__":
    main()

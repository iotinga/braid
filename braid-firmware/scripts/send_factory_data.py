import argparse
import logging
from dataclasses import dataclass
from pathlib import Path

import serial


@dataclass
class Args:
    port: Path
    bin: Path
    root_ca: Path
    device_cert: Path | None
    no_wait: bool


parser = argparse.ArgumentParser(description="send factory data")
parser.add_argument("--port", "-p", type=Path, help="serial port to use for sending the file")
parser.add_argument("bin", type=Path, help="the factory data binary file")
parser.add_argument("root_ca", type=Path, help="the root CA in .pem format")
parser.add_argument(
    "--device-cert",
    type=Path,
    default=None,
    help="BE CAREFUL! If set, overwrites the device certificate. It is already stored in flash during ATECC enrollment",
)
parser.add_argument(
    "--no-wait",
    dest="no_wait",
    default=False,
    action="store_true",
    help="If not set, assume the device is already initialized",
)


def send_data(port: serial.Serial, cmd: str, data: bytes):
    cmd = f"{cmd} {len(data)}\r\n"
    port.write(cmd.encode("ascii"))
    port.write(data)
    response = port.read_until("COMMAND END\r\n".encode("ascii"))

    logging.info("RESPONSE:")
    response = response.decode("ascii")
    print(response)
    if "Status: Success" not in response:
        raise RuntimeError("invalid response")


def main():
    logging.basicConfig(level=logging.DEBUG)
    args = parser.parse_args(namespace=Args)

    factory_data = args.bin.read_bytes()
    root_ca = args.root_ca.read_bytes()

    with serial.Serial(port=str(args.port), baudrate=115200, timeout=10) as port:
        if not args.no_wait:
            logging.info("Waiting for device CLI to initialize")
            port.read_until(b"CLI started")

        send_data(port, "factory-write", factory_data)
        send_data(port, "root-ca-write", root_ca)

        if args.device_cert is not None:
            answer = input(
                "BE CAREFUL! Overwriting the device certificate may make it different from the one stored in the ATECC, which may break SSL\n. Continue? [y/n] "
            ).lower()
            while answer not in ["y", "n"]:
                if answer == "y":
                    send_data(port, "device-cert-write", factory_data)
                elif answer == "n":
                    pass


if __name__ == "__main__":
    main()

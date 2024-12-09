#!/usr/bin/env python
# Copyright 2020 Espressif Systems (Shanghai) Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import argparse
import binascii
import logging
import os
import re
import sys
from pathlib import Path
from sys import exit

import helper_scripts as hs
from pyasn1_modules import pem

supported_targets = {"esp32", "esp32s2", "esp32s3", "esp32c3"}
try:
    import esptool
except ImportError:  # cheat and use IDF's copy of esptool if available
    idf_path_env = os.getenv("IDF_PATH")
    idf_path = Path(idf_path_env)
    if not idf_path_env or not idf_path.exists():
        raise
    sys.path.insert(0, idf_path / "components" / "esptool_py" / "esptool")
    import esptool  # noqa: F401

SERIAL_RE = re.compile(r"((?:[0-9a-fA-F]{2}\s*){9})")
CSR_RE = re.compile(r"-+(?:BEGIN CERTIFICATE REQUEST)-+([\S\s]*)-+(?:END CERTIFICATE REQUEST)-+")


def main():
    parser = argparse.ArgumentParser(
        description="""Provision the ESP32 device with
        device_certificate and signer_certificate required for TLS authentication"""
    )

    parser.add_argument(
        "--signer-cert",
        dest="signer_cert",
        type=Path,
        required=True,
        metavar="relative/path/to/signer_cert.pem",
        help="relative path(from secure_cert_mfg.py) to signer certificate.",
    )

    parser.add_argument(
        "--signer-cert-private-key",
        dest="signer_privkey",
        type=Path,
        required=True,
        metavar="relative/path/to/signer-priv-key",
        help="relative path(from secure_cert_mfg.py) to signer certificate private key",
    )

    parser.add_argument(
        "--pwd",
        "--password",
        dest="password",
        metavar="[password]",
        help="the password associated with the private key",
    )

    parser.add_argument(
        "--port",
        "-p",
        dest="port",
        metavar="[port]",
        required=True,
        help="uart com port to which ESP device is connected",
    )

    parser.add_argument(
        "--target_chip", dest="target_chip", required=True, choices=supported_targets, help="Target ESP32 series chip"
    )

    parser.add_argument(
        "--i2c-sda-pin', '-sda_pin'",
        dest="i2c_sda_pin",
        default=21,
        type=int,
        help="The pin no of I2C SDA pin of esp32 to which atecc608 is connected, default = 21",
    )

    parser.add_argument(
        "--i2c-scl-pin', '-scl_pin'",
        dest="i2c_scl_pin",
        default=22,
        type=int,
        help="The pin no of I2C SCL pin of esp32 to which atecc608 is connected, default = 22",
    )

    parser.add_argument(
        "--type",
        "--print-atecc608-type",
        dest="print_atecc608_type",
        action="store_true",
        help="print type of atecc608 chip connected to your ESP device",
    )

    parser.add_argument(
        "--valid-for-years",
        dest="nva_years",
        default=40,
        type=int,
        help="number of years for which device cert is valid (from current year), default = 40",
    )

    parser.add_argument(
        "--no-wait",
        dest="no_wait",
        default=False,
        action="store_true",
        help="If not set, assume the device is already initialized",
    )

    parser.add_argument(
        "-l",
        "--log",
        dest="log_level",
        default="info",
        choices=["debug", "info", "warning", "error", "critical"],
        help="Set the logging level",
    )

    parser.add_argument(
        "--lock_slots",
        dest="lock_slots",
        default=False,
        action="store_true",
        help="Whether to lock the device and signer certificate slot for ATECC TrustCustom "
        "\nSlots shall be permanently locked if set to true",
    )
    args = parser.parse_args()

    formatter = hs.utils.MultiLineFormatter(fmt="%(asctime)s %(levelname)-7s %(message)s", datefmt="%H:%M:%S")
    log_handler = logging.StreamHandler()
    log_handler.setFormatter(formatter)
    logging.basicConfig(level=logging.getLevelName(args.log_level.upper()), handlers=[log_handler])

    init_mfg = hs.serial.cmd_interpreter(port=args.port)

    retval = init_mfg.wait_for_init(args.no_wait)
    if retval is not True:
        logging.info("CMD prompt timed out.")
        exit(0)

    retval = init_mfg.exec_cmd(args.port, f"init {args.i2c_sda_pin} {args.i2c_scl_pin}")
    hs.serial.esp_cmd_check_ok(retval, f"init {args.i2c_sda_pin} {args.i2c_scl_pin}")

    if "TrustCustom" in retval[1]["Return"]:
        logging.info("ATECC608 chip is of type TrustCustom")
        provision_trustcustom_device(args, init_mfg)
    elif "Trust&Go" in retval[1]["Return"]:
        logging.info("ATECC608 chip is of type Trust&Go")
        hs.manifest.generate_manifest_file(args, init_mfg)
    elif "TrustFlex" in retval[1]["Return"]:
        logging.info("ATECC608 chip is of type TrustFlex")
        hs.manifest.generate_manifest_file(args, init_mfg)
    else:
        logging.info("Invalid type: ", retval[1]["Return"])
        exit(0)


def provision_trustcustom_device(args, init_mfg):
    output_files = Path("output_files")
    retval = init_mfg.exec_cmd(args.port, "print-chip-info")
    hs.serial.esp_cmd_check_ok(retval, "print-chip-info")

    match = SERIAL_RE.search(retval[1]["Return"])
    if match is None or len(match.groups()) == 0:
        logging.info("Error parsing serial number")
        exit(1)

    serial_number = bytearray.fromhex(match.group(1))
    serial_number_hex = (binascii.hexlify(serial_number)).decode()
    logging.info("Serial Number:")
    logging.info(serial_number_hex.upper())

    if args.print_atecc608_type is True:
        # print chip info and exit
        exit(0)
    logging.info("Provisioning the Device")
    retval = init_mfg.exec_cmd(args.port, "generate-keys 0")
    hs.serial.esp_cmd_check_ok(retval, "generate-keys")

    retval = init_mfg.exec_cmd(args.port, "generate-csr")
    hs.serial.esp_cmd_check_ok(retval, "generate-csr")

    csr = CSR_RE.search(retval[1]["Return"])
    if csr is None:
        logging.info("Device returned invalid CSR")
        exit(1)
    csr = csr[0].strip()
    logging.info("CSR obtained from device is:")
    logging.info(csr)

    try:
        # load private keys of signers to sign the CSR
        private_key = hs.cert_sign.load_privatekey(args.signer_privkey, args.password)
        signer_cert = hs.cert_sign.load_certificate(args.signer_cert)
        # Sign the CSR using the generated keys
        device_cert = hs.cert_sign.sign_csr(csr.encode(), signer_cert, private_key, serial_number_hex, args.nva_years)
        logging.info("Device cert generated:")
        dec_device_cert = device_cert.decode().strip()
        logging.info("Saving device cert to output_files/device_cert.pem")
        output_files.mkdir(exist_ok=True)

        if esp_handle_file(output_files / "device_cert.pem", "write", dec_device_cert) is not True:
            logging.info("Error in writing device certificate")
            exit(0)
        cert_der = esp_handle_file(output_files / "device_cert.pem", "pem_read")
    except ValueError:
        logging.info("Unsupported Key, Cert or CSR format specified.")
        exit(0)

    # get the cert definition and template data in string format
    logging.info("program device cert")
    slot_lock = 0

    if args.lock_slots:
        slot_lock = 1

    cert_def_str = hs.cert2certdef.esp_create_cert_def_str(cert_der, "DEVICE_CERT")

    retval = init_mfg.exec_cmd(args.port, f"provide-cert-def 0 {len(cert_def_str.encode('ascii'))}", cert_def_str)
    hs.serial.esp_cmd_check_ok(retval, "program-device-cert-def")

    retval = init_mfg.exec_cmd(args.port, f"program-dev-cert {slot_lock} {len(device_cert)}", device_cert)
    hs.serial.esp_cmd_check_ok(retval, f"program-dev-cert {slot_lock} {len(device_cert)}")

    signer_cert_data = esp_handle_file(args.signer_cert, "read").strip()
    cert_der = esp_handle_file(args.signer_cert, "pem_read")
    logging.info("Signer cert is:")
    logging.info(signer_cert_data)

    logging.info("Programming signer certificate")
    cert_def_str = hs.cert2certdef.esp_create_cert_def_str(cert_der, "SIGNER_CERT")

    retval = init_mfg.exec_cmd(args.port, f"provide-cert-def 1 {len(cert_def_str.encode('ascii'))}", cert_def_str)
    hs.serial.esp_cmd_check_ok(retval, "program-signer-cert-def")

    retval = init_mfg.exec_cmd(
        args.port, f"program-signer-cert {slot_lock} {len(signer_cert_data.encode('ascii'))}", signer_cert_data
    )
    hs.serial.esp_cmd_check_ok(retval, f"program-signer-cert {slot_lock}")


def esp_handle_file(file_path: Path, operation, data=None):
    if operation == "read":
        data = file_path.read_text()
        return data
    elif operation == "pem_read":
        with file_path.open() as cert_file:
            data = pem.readPemFromFile(cert_file)
        return data
    elif operation == "write":
        file_path.write_text(data)
        return True


if __name__ == "__main__":
    main()

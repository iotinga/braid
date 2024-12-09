#!/usr/bin/env python
# Copyright 2020 Espressif Systems (Shanghai) Co., Ltd.
#
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
import logging
import os
import subprocess
import sys
import time
from pathlib import Path
from sys import exit

import serial
from serial import SerialException

try:
    import esptool
except ImportError:  # cheat and use IDF's copy of esptool if available
    idf_path = Path(os.getenv("IDF_PATH"))
    if not idf_path or not Path.exists(idf_path):
        raise
    sys.path.insert(0, idf_path.joinpath("components", "esptool_py", "esptool"))
    import esptool


class cmd_interpreter:
    """
    This class is for is the command line interaction with the secure_cert_mfg firmware for manufacturing.
    It executes the specified commands and returns its result.
    It is a stateless, thus does not maintain the current state of the firmware.
    """

    def __init__(self, port, baudrate=115200):
        # Serial Port settings
        self.port = serial.Serial()
        self.port.timeout = 2
        self.port.baudrate = baudrate
        self.port.port = port
        logging.info(f"Connecting to {port}")
        while True:
            try:
                self.port.open()
                break
            except SerialException:
                pass
            time.sleep(1)

        self.port.close()

    def wait_for_init(self, no_wait=False):
        logging.info("Wait for init")
        if not self.port.isOpen():
            self.port.open()
        start_time = time.time()
        p_timeout = 20
        line = ""
        if not no_wait:
            while True:
                try:
                    line = self.port.readline()
                    if line:
                        logging.info(line.decode().strip())
                    if b"CLI started" in line:
                        logging.info("- Connected to CLI")
                        return True
                    elif (time.time() - start_time) > p_timeout:
                        logging.info("connection timed out")
                        return False
                except UnicodeError:
                    if line:
                        logging.info(line)
        else:
            return True

    def exec_cmd(self, port, command, args=None):
        ret = ""
        status = None
        logging.debug(f"Sending command {command}")
        self.port.timeout = 3
        self.port.write_timeout = 1
        self.port.write(command.encode("ascii") + b"\r\n")
        if args is not None:
            time.sleep(0.1)
            if type(args) is str:
                args = args.encode()

            self.port.write(args)
            logging.debug(f"Wrote {len(args)} bytes of args")

        while True:
            line = ""
            try:
                line = self.port.readline().decode()
                if line:
                    logging.info(line.strip() + "\033[0m")
            except UnicodeError:
                sys.stdout.flush()
                sys.stdout.write(line)

            if "Status: Success" in line:
                status = True
            elif "Status: Failure" in line:
                status = False
            if status is True or status is False:
                while True:
                    line = self.port.readline().decode()
                    if "COMMAND END" in line:
                        break
                    else:
                        ret += line
                return [{"Status": status}, {"Return": ret}]


def _exec_shell_cmd(self, command):
    result = subprocess.Popen((command).split(), stdout=subprocess.PIPE)
    out, err = result.communicate()
    return out


def get_load_ram_esptool_args(stub_path):
    class EsptoolArgs:
        def __init__(self, attributes):
            for key, value in attributes.items():
                self.__setattr__(key, value)

    esptool_args = EsptoolArgs({"filename": stub_path})
    return esptool_args


def load_app_stub(port, baudrate, stub_path):
    if stub_path is None:
        raise ValueError("Stub path cannot be None")
    esp = esptool.cmds.detect_chip(port=port, baud=baudrate)
    logging.info("Chip detected")
    esp.flash_spi_attach(0)

    esptool_args = get_load_ram_esptool_args(stub_path)
    start_time = time.time()
    esptool.cmds.load_ram(esp, esptool_args)
    end_time = time.time()
    logging.info(f"Time required to load the app into the RAM = {end_time - start_time}s")


def esp_cmd_check_ok(retval, cmd_str):
    logging.debug(f"Command '{cmd_str}' output: {retval}")
    if retval[0]["Status"] is not True:
        logging.info(cmd_str + " failed to execute")
        logging.info(retval[1]["Return"])
        exit(0)

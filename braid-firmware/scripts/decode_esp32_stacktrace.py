import argparse
import contextlib
import re
import subprocess
import sys
from pathlib import Path

DIR_PATH_RE = re.compile(r"(\/.*+)")
PREFIX_RE = re.compile(r"^ *")

parser = argparse.ArgumentParser(description="""Decode ESP32 backtrace""")
parser.add_argument("--environment", "-e", dest="env", help="The PlatformIO environment in use")
parser.add_argument(
    "--workdir", "-w", dest="work_dir", type=Path, help="The directory of the platformio.ini file in use"
)
parser.add_argument("backtrace", help="The backtrace as a string", nargs="*")


def main():
    args = parser.parse_args()
    pio_sys_info = subprocess.check_output(["pio", "system", "info"]).decode("utf-8")
    addr2line_path: Path | None = None
    for line in pio_sys_info.splitlines():
        if line.startswith("PlatformIO Core Directory"):
            match = DIR_PATH_RE.search(line)
            addr2line_path = (
                Path(match.group(1)) / "packages" / "toolchain-xtensa-esp32s3" / "bin" / "xtensa-esp32s3-elf-addr2line"
            )

    prefix_match = PREFIX_RE.match(line)
    prefix = prefix_match.group(0) if prefix_match is not None else ""

    elf_path = args.work_dir.absolute() / ".pio" / "build" / args.env / "firmware.elf"
    addr2line_args = [addr2line_path, "-fipC", "-e", elf_path]
    trace = ""
    try:
        i = 0
        for address in args.backtrace:
            output = subprocess.check_output(addr2line_args + [address]).decode("utf-8").strip()

            # newlines happen with inlined methods
            output = output.replace("\n", "\n     ")

            # throw out addresses not from ELF
            if output == "?? ??:0":
                continue

            func, file = output.split(" at ")

            with contextlib.suppress(ValueError):
                output = Path(output).relative_to(args.work_dir)
            trace += "%s  #%-2d %s in %-20s at %s\n" % (prefix, i, address, func, file)
            i += 1
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"failed to call {addr2line_path}: {e}\n")

    print(trace)


if __name__ == "__main__":
    main()

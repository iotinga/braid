import argparse
import hashlib
import json
import os
from pathlib import Path

parser = argparse.ArgumentParser(description="generates metadata JSON file")
parser.add_argument("--firmware", "-f", help="the built firmware file", type=Path)
parser.add_argument("--output", "-o", help="full output file path", type=Path)


def main():
    args = parser.parse_args()
    fw_hash = hashlib.sha256(args.firmware.read_bytes()).hexdigest()

    version_str = os.environ["IOTINGA_VERSION"]
    meta = {
        "version": version_str,
        "hash": fw_hash,
    }
    args.output.write_text(json.dumps(meta))


if __name__ == "__main__":
    main()

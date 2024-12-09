#!/usr/bin/env python3
# tool to easily generate partitions.csv with the correct alignment

import argparse
import logging
import sys
from dataclasses import dataclass

log = logging.getLogger(__name__)

FACTORY_PARTITION_SIZE_KB = 512

parser = argparse.ArgumentParser("gen_partition_table.py", description="Utility to generate the MCU partition table")
parser.add_argument("--flash-size", help="size of the flash (in MB)", type=int)
parser.add_argument(
    "--fill-gap",
    help="increase the size of the last partition to use all the available space",
    action="store_true",
)
parser.add_argument(
    "--output",
    type=argparse.FileType("w"),
    default=sys.stdout,
    help="output csv file to write",
)


@dataclass
class Partition:
    name: str
    type: str
    subtype: str
    size: int
    flags: str = ""


FIRMWARE_MAX_SIZE_KiB = 2048 + 64 * 4  # should be aligned to 64KiB for avoiding wasting flash in alignment
START_OFFSET = 0x10000
PARTITIONS = [
    Partition("factory", "app", "factory", FIRMWARE_MAX_SIZE_KiB),
    Partition("efuse_em", "data", "efuse", 8),
    Partition("factory-data", "data", "nvs", FACTORY_PARTITION_SIZE_KB),
    Partition("nvs", "data", "nvs", 1000),
]
ALIGNMENT = 0x10000  # 64KiB


class CsvWriter:
    def __init__(self, file):
        self.file = file

    def comment(self, comment: str):
        print("#", comment, file=self.file)

    def row(self, *args):
        print(*args, sep=",", file=self.file)


def main():
    logging.basicConfig(level=logging.DEBUG)
    args = parser.parse_args()

    out = CsvWriter(args.output)
    out.comment("WARNING: do not edit! autogenerated file.")
    out.comment(f'this was generated by using the command: {" ".join(sys.argv)}')
    out.comment("Name,Type,SubType,Offset,Size,Flags")

    flash_size = 1024 * 1024 * args.flash_size
    offset = START_OFFSET
    for index, partition in enumerate(PARTITIONS):
        partition.size *= 1024
        if index == len(PARTITIONS) - 1 and args.fill_gap:
            partition.size = flash_size - offset

        if partition.type == "app" and offset % ALIGNMENT != 0:
            align = ALIGNMENT - offset % ALIGNMENT
            out.comment(f"# adding {align // 1024}KiB to align the partition")
            offset += align

        out.comment(
            f"partition {partition.name} ({partition.type}:{partition.subtype}) of size {partition.size // 1024}KiB at offset {offset // 1024}KiB ({hex(offset)})"
        )
        out.row(
            partition.name,
            partition.type,
            partition.subtype,
            hex(offset),
            partition.size,
            partition.flags,
        )

        offset += partition.size

    if offset > flash_size:
        log.warn(f"partition table bigger than flash size: {offset // 1024}KiB > {flash_size // 1024}KiB")
    if offset < flash_size:
        log.warn(f"WARNING: {(flash_size - offset)}KiB unused at the end of the partition")


if __name__ == "__main__":
    main()

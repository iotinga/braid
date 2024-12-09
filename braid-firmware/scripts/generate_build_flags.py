import os
import re
import subprocess

SEMVER_RE = re.compile(
    r"([0-9]+)\.([0-9]+)\.([0-9]+)(?:(\-[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?(?:\+[0-9A-Za-z-\-\.]+)?"
)

"""
Generates build flags for PlatformIO. By default generates the version defines by parsing git rev information. 
Also reads the `EXTRA_FLAGS` environment variable and adds those flags to the compiler command
"""


class Version:
    major: int
    minor: int
    patch: int
    metadata: str

    def __init__(self, version_str: str):
        match = SEMVER_RE.match(version_str)
        if not match:
            raise "Invalid version string"

        self.major = int(match.group(1))
        self.minor = int(match.group(2))
        self.patch = int(match.group(3))
        self.metadata = match.group(4)


def main():
    version_str = os.environ.get("IOTINGA_VERSION", "0.0.0")
    extra_flags: str = os.environ.get("EXTRA_FLAGS", "")
    build_ver = Version(version_str.strip("v"))
    commit_short = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).decode("ascii").strip()

    version_defines = {
        "CFG_FW_VERSION_MAJOR": build_ver.major,
        "CFG_FW_VERSION_MINOR": build_ver.minor,
        "CFG_FW_VERSION_PATCH": build_ver.patch,
        "CFG_FW_VERSION_COMMIT": commit_short,
    }

    version_defines_str = " ".join([f"-D{flag}={value}" for flag, value in version_defines.items()])
    print(version_defines_str + " " + extra_flags)


if __name__ == "__main__":
    main()

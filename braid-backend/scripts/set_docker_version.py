#!/usr/bin/env python3
# Updates the image version in the compose.yaml

import re
import argparse
from pathlib import Path

ROOT = Path(__file__).parent.parent
DOCKER_COMPOSE_PATHS = [
    ROOT / "compose.yaml",
]

DOCKER_REPOSITORY_IMAGE_HINT = 'iotinga-docker.nexus.tinga.io'

parser = argparse.ArgumentParser(description="script to set the docker-compose file version")
parser.add_argument("version", help="version to set")


def main():
    args = parser.parse_args()

    for docker_compose_path in DOCKER_COMPOSE_PATHS:
        with open(docker_compose_path, "r") as f:
            compose = f.read()

        matches = re.findall(DOCKER_REPOSITORY_IMAGE_HINT+"/.*:[0-9]+\.[0-9]+\.[0-9]+", compose)
        for match in matches:
            prefix = match.split(":")[0]
            compose = compose.replace(match, f"{prefix}:{args.version}")

        with open(docker_compose_path, "w") as f:
            f.write(compose)


if __name__ == "__main__":
    main()
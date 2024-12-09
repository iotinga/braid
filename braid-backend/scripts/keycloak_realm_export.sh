#!/bin/bash

compose_network="braid-backend-compose_default"
keycloak_ver="25.0"

if ! docker network ls --format json | grep "$compose_network" &>/dev/null; then
  echo "Compose network not found! Is the stack running?"
  exit 1
fi

if ! docker ps --format json | grep keycloak &>/dev/null; then
  echo "Keycloak container not found"
  exit 1
fi

docker compose down keycloak

docker run \
  --rm \
  --name keycloak-export \
  --volume "$(pwd)":/tmp/keycloak \
  --network $compose_network \
  keycloak/keycloak:$keycloak_ver \
  export --dir /tmp/keycloak/export/ --realm demo --users different_files \
  --db postgres \
  --db-username keycloak \
  --db-password abram.space \
  --db-url jdbc:postgresql://timescaledb:5432/keycloak

docker compose up -d keycloak
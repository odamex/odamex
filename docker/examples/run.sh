#!/usr/bin/env bash
if [[ ! -d ./odahome ]]; then
    mkdir odahome
fi
UP_DOWN=${1:-up -d}
DOCKER_COMPOSE=$(which docker-compose 2>/dev/null)
if [[ -z "$DOCKER_COMPOSE" ]]; then
    DOCKER_COMPOSE="docker compose"
fi
$DOCKER_COMPOSE -p odamex ${UP_DOWN}

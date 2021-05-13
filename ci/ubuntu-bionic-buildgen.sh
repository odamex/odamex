#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

docker build -t odamex -f ci/ubuntu-bionic.Dockerfile .
docker run --rm odamex

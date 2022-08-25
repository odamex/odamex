#!/usr/bin/env zsh

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

set -x

# Install packages
# Ensure to install sdl 2.0.22, as Homebrew latest is 2.24.0
curl https://raw.githubusercontent.com/Homebrew/homebrew-core/3a9d188a695a38244c42e68117aff412ae0884eb/Formula/sdl2.rb > $(find $(brew --repository) -name sdl2.rb) && brew install sdl2
brew install sdl2_mixer wxmac

# Generate build
mkdir -p build && cd build
cmake .. -GXcode \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1

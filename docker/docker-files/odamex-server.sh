#!/usr/bin/env bash

# Set the working directory to the Odamex installation dir. This is so it can find its own files,
# like odamexa.wad, etc.
cd "INSTALL_DIR"
exec "INSTALL_DIR/odasrv" "$@"

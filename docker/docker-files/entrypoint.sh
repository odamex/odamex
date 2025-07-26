#!/usr/bin/env bash
set -exu

# Do not allow container to be started as non-root user
if (( "$(id -u)" != 0 )); then
    echo "You must run the container as root. To specify a custom user,"
    echo "use the ODAMEX_UID and ODAMEX_GID environment variables"
    exit 1
fi

 # Create the group for the server process
[[ -n "$ODAMEX_GID" ]] && GID_OPTION="--gid $ODAMEX_GID"
groupadd odamex --force ${GID_OPTION-}

# Create the user for the server process
[[ -n "$ODAMEX_UID" ]] && UID_OPTION="--uid $ODAMEX_UID"
useradd doomguy --create-home ${UID_OPTION-} \
    --shell /sbin/nologin \
    --group odamex \
    || true # Do not fail if user already exists

# Start the odamex-server process with local user & group
gosu doomguy:odamex odamex-server -host "$@"

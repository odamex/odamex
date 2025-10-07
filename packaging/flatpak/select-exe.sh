#!/bin/bash
odamex_client() {
    command odamex "$@"
}

odamex_server() {
    command odasrv "$@"
}

odamex_launcher() {
    command odalaunch "$@"
}

if declare -f "odamex_$1" >/dev/null; then
    func="odamex_$1"
    shift
    "$func" "$@"
else
    echo -e "\e[4mUsage:\e[0m flatpak run net.odamex.Odamex <COMMAND>"
    echo
    echo -e "\e[4mCommands:\e[0m"
    echo "  client"
    echo "  server"
    echo "  launcher"
    exit 1
fi

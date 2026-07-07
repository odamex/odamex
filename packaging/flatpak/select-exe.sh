#!/bin/bash
odamex_client() {
    exec odamex "$@"
}

odamex_server() {
    exec odasrv "$@"
}

odamex_launcher() {
    exec odalaunch "$@"
}

odamex_shell-integration() {
    local shell="${1-}"
    case "$shell" in
        "")
            printf "\e[4mUsage:\e[0m flatpak run net.odamex.Odamex shell-integration <SHELL>\n\n"
            printf "\e[4mShells:\e[0m\n"
            printf "  bash  - Tab completions and aliases for GNU Bash\n"
            printf "  sh    - Basic aliases for POSIX compatible shells\n"
            exit 1
            ;;
        bash)
            cat <<'EOF'
###################################################################################
# Odamex Bash integration
#
# Add this line to ~/.bashrc:
#   source <(flatpak run net.odamex.Odamex shell-integration bash)
#
# Or save to a file:
#   mkdir -p ~/.bashrc.d
#   flatpak run net.odamex.Odamex shell-integration bash > ~/.bashrc.d/odamex.bash
#   source ~/.bashrc.d/odamex.bash
###################################################################################

EOF
            cat /app/share/odamex/shell/posix_aliases.sh \
                /app/share/bash-completion/completions/odamex.bash
            ;;
        posix|sh)
            cat /app/share/odamex/shell/posix_aliases.sh
            ;;
        *)
            echo "Unknown shell: ${shell}"
            exit 1
            ;;
    esac
}

if declare -F "odamex_$1" > /dev/null; then
    func="odamex_$1"
    shift
    "$func" "$@"
else
    printf "\e[4mUsage:\e[0m flatpak run net.odamex.Odamex <COMMAND>\n\n"
    printf "\e[4mCommands:\e[0m\n"
    printf "  client\n"
    printf "  server\n"
    printf "  launcher\n"
    printf "  shell-integration {bash,sh}\n"
    exit 1
fi

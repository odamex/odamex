if ! command -v odamex > /dev/null 2>&1; then
    odamex() {
        flatpak run net.odamex.Odamex client "$@"
    }
fi

if ! command -v odasrv > /dev/null 2>&1; then
    odasrv() {
        flatpak run net.odamex.Odamex server "$@"
    }
fi

if ! command -v odalaunch > /dev/null 2>&1; then
    odalaunch() {
        flatpak run net.odamex.Odamex launcher "$@"
    }
fi

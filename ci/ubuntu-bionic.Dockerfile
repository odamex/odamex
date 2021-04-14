FROM ubuntu:18.04

WORKDIR odamex

COPY . .

# Packages - first the majority of them, then cmake
RUN set -x && \
    apt update && \
    apt install -y ninja-build libsdl2-dev libsdl2-mixer-dev \
        libpng-dev libcurl4-openssl-dev libwxgtk3.0-gtk3-dev deutex \
        apt-transport-https ca-certificates gnupg software-properties-common wget && \
    wget -O - 'https://apt.kitware.com/keys/kitware-archive-latest.asc' 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' && \
    apt update && apt install -y cmake

WORKDIR build

# Build commands
RUN cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1

CMD ["ninja"]

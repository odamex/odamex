FROM fedora:28

WORKDIR odamex

COPY . .

# Packages
RUN set -x && \
    dnf -y install gcc-c++ alsa-lib-devel libcurl-devel \
                   ninja-build SDL2-devel SDL2_mixer-devel wxGTK3-devel && \
   curl -LO https://github.com/Kitware/CMake/releases/download/v3.30.2/cmake-3.30.2-linux-x86_64.sh && \
   chmod +x ./cmake-3.30.2-linux-x86_64.sh && \
   ./cmake-3.30.2-linux-x86_64.sh --skip-license --prefix=/usr/local

WORKDIR build

# Build commands
RUN cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_MINIUPNP=0 \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1 \
    -DwxWidgets_wxrc_EXECUTABLE=/usr/bin/wxrc-3.0

CMD ["ninja"]

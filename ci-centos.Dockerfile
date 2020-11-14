FROM centos:7

WORKDIR odamex

COPY . .

# Packages
RUN set -x && \
    yum -y install epel-release gcc-c++ alsa-lib-devel libcurl-devel && \
    yum -y install cmake3 ninja-build SDL2-devel SDL2_mixer-devel wxGTK3-devel

WORKDIR build

# Build commands
RUN cmake3 .. -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_MINIUPNP=0 \
    -DBUILD_OR_FAIL=1 -DBUILD_CLIENT=1 -DBUILD_SERVER=1 \
    -DBUILD_MASTER=1 -DBUILD_LAUNCHER=1 \
    -DwxWidgets_wxrc_EXECUTABLE=/usr/bin/wxrc-3.0

CMD ["ninja"]

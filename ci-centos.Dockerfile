FROM centos:7

WORKDIR odamex

COPY . .

# Packages
RUN set -x && \
    yum -y install epel-release gcc-c++ alsa-lib-devel && \
    yum -y install cmake3 ninja-build SDL2-devel SDL2_mixer-devel

WORKDIR build

# Build commands
RUN cmake3 .. -GNinja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_MINIUPNP=0

CMD ["ninja"]

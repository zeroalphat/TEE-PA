FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install --no-install-recommends -y android-tools-adb \
    android-tools-fastboot\
    autoconf \
    automake \
    bc \
    bison \
    build-essential \
    ccache\
    codespell \
    cscope\
    curl\
    device-tree-compiler\
    expect\ 
    flex\ 
    ftp-upload\ 
    gdisk\ 
    iasl\ 
    libattr1-dev\ 
    libcap-dev\
    libcap-ng-dev \
    libfdt-dev\
    libftdi-dev\
    libglib2.0-dev\
    libgmp-dev\
    libhidapi-dev \
    libmpc-dev\
    libncurses5-dev\
    libpixman-1-dev \
    libssl-dev\
    libtool \
    make \
    mtools \
    netcat \
    ninja-build \
    python-crypto \
    python3-crypto \
    python-pyelftools \
    python3-pycryptodome \
    python3-pyelftools \
    python-serial \
    python3-serial \
    rsync \
    unzip \
    uuid-dev \
    xdg-utils \
    xterm \
    xz-utils \
    zlib1g-dev \
    git \
    ssh \
    wget \
    cpio \
    ca-certificates \
    && apt-get -y clean \
    && rm -rf /var/lib/apt/lists/*

RUN curl https://storage.googleapis.com/git-repo-downloads/repo > /usr/bin/repo \
    && chmod a+x /usr/bin/repo

WORKDIR /app/optee
RUN repo init -u https://github.com/OP-TEE/manifest.git -m qemu_v8.xml -b 3.11.0 \
    && repo sync -j4

WORKDIR /app/optee/build
RUN make -j2 toolchains

# qemu patch 
COPY qemu.patch /app/optee/build
RUN patch -p1 -l < qemu.patch

# add TA and PTA to optee directory
COPY ./optee_examples /app/optee/optee_examples
COPY ./pta /app/optee/optee_os/core/pta
COPY ./pta/include /app/optee/optee_os/lib/libutee/include

# customize linux kernel
COPY ./linux  /app/optee/linux
COPY ./linux/optee_private.h /app/optee/linux/kernel
COPY ./linux/optee_msg.h /app/optee/linux/kernel
WORKDIR /app/optee/linux 
RUN patch -p1 -l < audit_qemu.patch
RUN patch -p1 -l < show_memory_layout_on_boot.patch  

WORKDIR /app/optee/build
RUN make -j12 CFG_CORE_DYN_SHM=n CFG_CRYPTOLIB_NAME=mbedtls CFG_CRYPTOLIB_DIR=lib/libmbedtls all

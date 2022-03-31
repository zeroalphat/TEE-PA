FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install --no-install-recommends -y software-properties-common \
    && rm -rf /var/lib/apt/lists/*

RUN add-apt-repository -y ppa:openjdk-r/ppa \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
    openjdk-11-jdk \
    auditd \
    bison  \
    clang  \
    cmake  \
    curl  \
    flex  \
    fuse  \
    git  \
    ifupdown  \
    libaudit-dev  \
    libfuse-dev  \
    lsof  \
    pkg-config  \
    unzip  \
    uthash-dev \ 
    wget \
    gcc \
    && apt-get -y clean \
    && rm -rf /var/lib/apt/lists/*

# build SPADE
WORKDIR /app
RUN git clone https://github.com/ashish-gehani/SPADE.git 
WORKDIR /app/SPADE
RUN git checkout 8d51f11a7a5aa7db4fa5ddabcf13e672881f2ab6
COPY spade_src.patch /app/SPADE/
RUN patch -p1 -l < spade_src.patch \
    && sh -c "./configure"

RUN make KERNEL_MODULES=false
COPY init-spade.sh /app
COPY spade.config /app/SPADE


#ENTRYPOINT ["/app/docker-entrypoint.sh"]
#CMD ["/app/SPADE/bin/spade", "debug"]

FROM ubuntu:21.04 AS base

# Install everything except cmake
RUN apt-get update && \
  apt-get -y upgrade &&\
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get install -y \
    apt-transport-https \
    build-essential \
    ca-certificates \
    fdkaac \
    git \
    gnupg \
    gnuradio \
    gnuradio-dev \
    gr-osmosdr \
    libboost-all-dev \
    libcurl4-openssl-dev \
    libgmp-dev \
    libhackrf-dev \
    liborc-0.4-dev \
    libpthread-stubs0-dev \
    libssl-dev \
    libuhd-dev \
    libusb-dev \
    pkg-config \
    software-properties-common \
    cmake \
    sox && \
  rm -rf /var/lib/apt/lists/*

# Need to install newer cmake than what's in Ubuntu repo to build armv7 due to this:
# https://gitlab.kitware.com/cmake/cmake/-/issues/20568
#RUN curl https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
#    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main' && \
#    apt-get update && \
#    export DEBIAN_FRONTEND=noninteractive && apt-get install -y cmake && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

RUN cmake . && make -j`nproc` && cp -i recorder /usr/local/bin

#USER nobody

WORKDIR /app

# GNURadio requires a place to store some files, can only be set via $HOME env var.
ENV HOME=/tmp

CMD ["/usr/local/bin/recorder", "--config=/app/config.json"]

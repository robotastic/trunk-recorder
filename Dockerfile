FROM ubuntu:22.04 AS base

# Install docker for passing the socket to allow for intercontainer exec
RUN apt-get update && \
  apt-get -y upgrade &&\
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get install -y \
    apt-transport-https \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    docker.io \
    fdkaac \
    git \
    gnupg \
    gnuradio \
    gnuradio-dev \
    gr-funcube \
    gr-iqbal \
    libairspy-dev \
    libairspyhf-dev \
    libbladerf-dev \
    libboost-all-dev \
    libcurl4-openssl-dev \
    libfreesrp-dev \
    libgmp-dev \
    libhackrf-dev \
    libmirisdr-dev \
    liborc-0.4-dev \
    libpthread-stubs0-dev \
    libsndfile1-dev \
    libsoapysdr-dev \
    libssl-dev \
    libuhd-dev \
    libusb-1.0-0-dev \
    libxtrx-dev \
    pkg-config \
    software-properties-common \
    sox \
    wget && \
  rm -rf /var/lib/apt/lists/*

# Fix the error message level for SmartNet

RUN sed -i 's/log_level = debug/log_level = info/g' /etc/gnuradio/conf.d/gnuradio-runtime.conf

# Compile librtlsdr-dev 2.0 for SDR-Blog v4 support and other updates
# Ubuntu 22.04 LTS has librtlsdr 0.6.0
RUN cd /tmp && \
  git clone https://github.com/steve-m/librtlsdr.git && \
  cd librtlsdr && \
  mkdir build && \
  cd build && \
  cmake .. && \
  make -j$(nproc) && \
  make install && \
  ldconfig && \
  cd /tmp && \
  rm -rf librtlsdr

# Compile gr-osmosdr ourselves using a fork with various patches included
RUN cd /tmp && \
  git clone https://github.com/racerxdl/gr-osmosdr.git && \
  cd gr-osmosdr && \
  mkdir build && \
  cd build && \
  cmake -DENABLE_NONFREE=TRUE .. && \
  make -j$(nproc) && \
  make install && \
  ldconfig && \
  cd /tmp && \
  rm -rf gr-osmosdr

WORKDIR /src

COPY . .

WORKDIR /src/build

RUN cmake .. && make -j$(nproc) && make install

#USER nobody

WORKDIR /app

# GNURadio requires a place to store some files, can only be set via $HOME env var.
ENV HOME=/tmp

CMD trunk-recorder --config=/app/config.json

FROM ubuntu:21.04 AS base

# Install everything except cmake
# Install docker for passing the socket to allow for intercontainer exec
RUN apt-get update && \
  apt-get -y upgrade &&\
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get install -y \
    apt-transport-https \
    build-essential \
    ca-certificates \
    fdkaac \
    git \
    docker \
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

WORKDIR /src

COPY . .

WORKDIR /src/build

RUN cmake .. && make && make install

#USER nobody

WORKDIR /app

# GNURadio requires a place to store some files, can only be set via $HOME env var.
ENV HOME=/tmp

CMD trunk-recorder --config=/app/config.json

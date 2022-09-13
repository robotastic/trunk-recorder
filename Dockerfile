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
    liborc-0.4-dev \
    libpthread-stubs0-dev \
    librtlsdr-dev \
    libsndfile1-dev \
    libsoapysdr-dev \
    libssl-dev \
    libuhd-dev \
    libusb-dev \
    libxtrx-dev \
    pkg-config \
    software-properties-common \
    sox && \
  rm -rf /var/lib/apt/lists/*

COPY lib/gr-osmosdr/airspy_source_c.cc.patch /tmp/airspy_source_c.cc.patch

# Compile gr-osmosdr ourselves so we can install the Airspy serial number patch
RUN cd /tmp && \
  git clone https://git.osmocom.org/gr-osmosdr && \
  cd gr-osmosdr && \
  git apply ../airspy_source_c.cc.patch && \
  mkdir build && \
  cd build && \
  cmake .. && \
  make -j$(nproc) && \
  make install && \
  ldconfig && \
  cd /tmp && \
  rm -rf gr-osmosdr airspy_source_c.cc.patch

WORKDIR /src

COPY . .

WORKDIR /src/build

RUN cmake .. && make && make install

#USER nobody

WORKDIR /app

# GNURadio requires a place to store some files, can only be set via $HOME env var.
ENV HOME=/tmp

CMD trunk-recorder --config=/app/config.json

# docker build -t robotastic/trunk-recorder:latest .

FROM ubuntu:20.04 AS base

RUN apt-get update
RUN apt-get install -y gnuradio gnuradio-dev gr-osmosdr libhackrf-dev libuhd-dev libgmp-dev
RUN apt-get install -y git cmake build-essential pkg-config libboost-all-dev libusb-1.0-0.dev libssl-dev libcurl4-openssl-dev liborc-0.4-dev
# install deps for gnuradio audio
RUN apt-get install -y libasound2-dev libjack-dev portaudio19-dev
RUN apt-get install -y ca-certificates expat libgomp1 fdkaac sox
# install necessary locales
RUN apt-get install -y locales \
    && echo "en_US.UTF-8 UTF-8" > /etc/locale.gen \
    && locale-gen

RUN apt-get autoremove -y && \
    apt-get clean -y

RUN mkdir /src
WORKDIR /src

ARG ARCH="x86_64"
ARG VERS="2.13"
ADD https://www.sdrplay.com/software/SDRplay_RSP_API-Linux-2.13.1.run /src/SDRplay_RSP_API-Linux-2.13.1.run
RUN chmod +x ./SDRplay_RSP_API-Linux-2.13.1.run \
 && ./SDRplay_RSP_API-Linux-2.13.1.run --noexec --target /src/sdrplay \
 && cd sdrplay \
 && cp ${ARCH}/libmirsdrapi-rsp.so.${VERS} /usr/local/lib/. \
 && chmod 644 /usr/local/lib/libmirsdrapi-rsp.so.${VERS} \
 && ln -s /usr/local/lib/libmirsdrapi-rsp.so.${VERS} /usr/local/lib/libmirsdrapi-rsp.so.2 \
 && ln -s /usr/local/lib/libmirsdrapi-rsp.so.2 /usr/local/lib/libmirsdrapi-rsp.so \
 && cp mirsdrapi-rsp.h /usr/local/include/. \
 && chmod 644 /usr/local/include/mirsdrapi-rsp.h

RUN apt-get install -y \
    soapysdr-tools \
    libsoapysdr-dev \
    soapysdr-module-all \
# To avoid avahi dependency
 && rm /usr/lib/x86_64-linux-gnu/SoapySDR/modules0.7/libremoteSupport.so

RUN git clone https://github.com/dlaw/SoapySDRPlay.git \
 && cd SoapySDRPlay \
 && git checkout cleanup \
 && cmake . \
 && make -j`nproc` \
 && make install \
 && ldconfig

COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make -j`nproc`
RUN mkdir /app && cp /src/trunk-recorder/recorder /app

RUN mkdir /app/media \
 && mkdir /app/config

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

WORKDIR /app
CMD ["./recorder","--config=/app/config/config.json"]

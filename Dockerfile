# docker build -t robotastic/trunk-recorder:latest .

FROM ubuntu:18.04 AS base

RUN apt-get update
RUN apt-get install -y gnuradio gnuradio-dev gr-osmosdr libhackrf-dev libuhd-dev
RUN apt-get install -y git cmake build-essential libboost-all-dev libusb-1.0-0.dev libssl-dev libcurl4-openssl-dev
RUN apt-get install -y ca-certificates expat libgomp1 fdkaac sox
# install necessary locales
RUN apt-get install -y locales \
    && echo "en_US.UTF-8 UTF-8" > /etc/locale.gen \
    && locale-gen

RUN apt-get autoremove -y && \
    apt-get clean -y

COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make
RUN mkdir /app && cp /src/trunk-recorder/recorder /app

RUN mkdir /app/media \
 && mkdir /app/config

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

WORKDIR /app
CMD ["./recorder","--config=/app/config/config.json"]

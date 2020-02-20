# docker build -t robotastic/trunk-recorder:latest .

FROM registry.gitlab.com/erictendian/docker-gnuradio/master:latest

RUN apt-get update && apt-get upgrade -y

COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make
RUN mkdir /app && cp /src/trunk-recorder/recorder /app

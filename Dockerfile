# docker build -t robotastic/trunk-recorder:latest .

FROM registry.gitlab.com/erictendian/docker-gnuradio/master:latest

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y libcurl4-gnutls-dev

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

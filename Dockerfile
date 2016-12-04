FROM robotastic/docker-gnuradio

RUN apt-get update && apt-get upgrade -y

COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make

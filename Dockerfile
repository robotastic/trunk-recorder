FROM robotastic/docker-gnuradio:latest

COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make

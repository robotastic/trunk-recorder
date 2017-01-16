# docker build -t robotastic/trunk-recorder:latest .

FROM robotastic/docker-gnuradio:latest



COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make
RUN mkdir /app && cp /src/trunk-recorder/recorder /app

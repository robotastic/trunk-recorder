FROM robotastic/docker-gnuradio:latest

RUN echo deb http://www.deb-multimedia.org jessie main non-free \
	>>/etc/apt/sources.list && apt-get update &&\
	apt-get install -y --force-yes deb-multimedia-keyring && \
	apt-get update && \
	apt-get install -y ffmpeg

COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make
RUN mkdir /app && cp /src/trunk-recorder/recorder /app

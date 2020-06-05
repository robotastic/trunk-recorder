FROM robotastic/docker-gnuradio:latest

WORKDIR /src

COPY . .

RUN cmake . && make -j`nproc` && cp recorder /recorder

CMD ["/recorder"]

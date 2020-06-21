FROM robotastic/docker-gnuradio:latest

WORKDIR /src

COPY . .

RUN cmake . && make -j`nproc` && cp recorder /recorder

USER nobody

CMD ["/recorder", "--config=/app/config.json"]

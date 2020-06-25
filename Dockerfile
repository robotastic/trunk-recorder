FROM robotastic/gnuradio:latest

WORKDIR /src

COPY . .

RUN cmake . && make && cp recorder /recorder

#USER nobody

CMD ["/recorder", "--config=/app/config.json"]

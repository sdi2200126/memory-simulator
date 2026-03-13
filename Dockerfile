FROM ubuntu:latest AS builder

RUN apt-get update && apt-get install -y gcc make
COPY . .
RUN make

FROM ubuntu:latest
COPY --from=builder /bin ./bin
COPY --from=builder /traces ./traces

CMD ["./bin/MM","-k","10","-f","200","-q","10"]
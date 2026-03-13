FROM ubuntu:26.04 AS builder

RUN apt-get update && apt-get install -y gcc make libc6-dev
COPY . .
RUN make

FROM ubuntu:26.04
COPY --from=builder /bin ./bin
COPY --from=builder /traces ./traces

CMD ["./bin/MM","-k","10","-f","200","-q","10"]
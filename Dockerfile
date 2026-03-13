FROM ubuntu:latest

RUN apt-get update && apt-get install -y gcc make

COPY . .

CMD ["make", "run"]
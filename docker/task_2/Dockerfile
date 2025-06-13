FROM ubuntu:16.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y build-essential && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY main.cpp .

RUN g++ -std=c++17 -pthread -O2 -o parallel_sum main.cpp

CMD ["./parallel_sum"]
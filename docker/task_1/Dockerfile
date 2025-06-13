FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y build-essential && \
    rm -rf /var/lib/apt/lists/*

 WORKDIR /app
 COPY hello.cpp .

 RUN g++ -o hello hello.cpp

 CMD ["./hello"]
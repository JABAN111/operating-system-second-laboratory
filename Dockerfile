FROM ubuntu:20.04

LABEL authors="jaba"

ENV TZ=Europe/Moscow
ARG DEBIAN_FRONTEND=noninteractive


RUN apt-get update

RUN apt-get install -y g++ make wget tar libssl-dev

#building cmake 3.28 version
RUN wget https://github.com/Kitware/CMake/archive/refs/tags/v3.28.3.tar.gz
RUN tar zxvf v3.28.3.tar.gz
RUN ./CMake-3.28.3/bootstrap
RUN make
RUN make install

WORKDIR /app

COPY . .

RUN rm -fr build
RUN mkdir build && cd build && cmake .. && make
RUN #cmake ..
RUN #make


FROM ubuntu:22.04

ARG UID="1000"
ARG GID="1000"

RUN apt-get update -y 

RUN    apt-get clean && \
    apt-get install --no-install-recommends -y \
        sudo \
        nano \
        vim \
        wget \
        git \
        cmake \
        build-essential \
        ninja-build \
        gcc-arm-none-eabi \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib \
        python3 \
        python3-pip \
        python3-venv && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN addgroup --gid $GID ubuntu && \
    adduser --uid $UID --gid $GID --disabled-password --gecos "" ubuntu && \
    echo 'ubuntu ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers

# Set the non-root user as the default user
USER ubuntu
WORKDIR /home/ubuntu

RUN git clone --recursive https://github.com/raspberrypi/pico-sdk.git && \
    echo 'export PICO_SDK_PATH=/home/ubuntu/pico-sdk' >> ~/.bashrc

ENV PICO_SDK_PATH=/home/ubuntu/pico-sdk

CMD ["/bin/bash"]
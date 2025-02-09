FROM ubuntu:24.10

RUN apt-get update -y && \
    apt-get clean && \
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

USER ubuntu
WORKDIR /home/ubuntu

RUN git clone --recursive https://github.com/raspberrypi/pico-sdk.git && \
    echo 'export PICO_SDK_PATH=/home/ubuntu/pico-sdk' >> ~/.bashrc

ENV PICO_SDK_PATH=/home/ubuntu/pico-sdk

CMD ["/bin/bash"]
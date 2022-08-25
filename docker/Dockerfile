FROM ubuntu:20.04

ENV LD_LIBRARY_PATH=/opt/bladerf/host/build/output

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata \
    && apt-get -y install git libusb-1.0-0-dev libusb-1.0-0 build-essential cmake libncurses5-dev libtecla1 libtecla-dev pkg-config

RUN git clone https://github.com/Nuand/bladeRF.git /opt/bladerf \
    && mkdir -p /opt/bladerf/host/build \
    && cd /opt/bladerf/host/build \
    && cmake .. \
    && make

RUN git clone https://github.com/osqzss/bladeGPS.git /opt/bladegps \
    && cd /opt/bladegps \
    && make

ENV PATH="/opt/bladegps:${PATH}"

WORKDIR /bladegps



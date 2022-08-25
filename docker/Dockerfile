FROM ubuntu:20.04

RUN adduser --system --home /bladegps --disabled-password --group bladegps

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata \
    && apt-get -y install git libusb-1.0-0-dev libusb-1.0-0 build-essential cmake libncurses5-dev libtecla1 libtecla-dev pkg-config \
    && chmod 775 /opt \
    && mkdir -p /opt/bladeRF \
    && chown -R bladegps:bladegps /opt/bladeRF \
    && mkdir -p /opt/bladeGPS \
    && chown -R bladegps:bladegps /opt/bladeGPS

USER bladegps

ENV LD_LIBRARY_PATH=/opt/bladeRF/host/build/output

RUN git clone https://github.com/Nuand/bladeRF.git /opt/bladeRF \
    && mkdir -p /opt/bladeRF/host/build \
    && cd /opt/bladeRF/host/build \
    && cmake .. \
    && make

RUN git clone https://github.com/osqzss/bladeGPS.git /opt/bladeGPS \
    && cd /opt/bladeGPS \
    && make

ENV PATH="/opt/bladeGPS:${PATH}"

WORKDIR /bladegps

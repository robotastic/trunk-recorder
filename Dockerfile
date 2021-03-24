FROM debian:buster-slim AS deps

WORKDIR /src

RUN apt-get update && \
  apt-get -y upgrade &&\
  # set noninteractive installation
  export DEBIAN_FRONTEND=noninteractive && \
  #install tzdata package
  apt-get install -y \
    airspy \
    build-essential \
    cmake \
    curl \
    g++ \
    git \
    libairspy-dev \
    libairspy0 \
    libairspyhf-dev \
    libairspyhf1 \
    libcppunit-dev \
    libcurl4 \
    libcurl4-openssl-dev \
    libfftw3-dev \
    libgmp-dev \
    libhackrf-dev \
    libhackrf0 \
    liblog4cpp5-dev \
    liblog4cpp5v5 \
    librtlsdr-dev \
    librtlsdr0 \
    libsoapysdr-dev \
    libssl-dev \
    libusb-1.0-0 \
    libusb-dev \
    pkg-config \
    python3-dev \
    python3-distutils \
    python3-mako \
    python3-numpy \
    python3-six \
    rtl-sdr && \
  rm -rf /var/lib/apt/lists/*

## Boost
ARG BOOST_VERSION=1_75_0
ARG BOOST_VERSION_DOT=1.75.0
ARG BOOST_HASH=953db31e016db7bb207f11432bef7df100516eeb746843fa0486a222e3fd49cb
RUN set -ex \
    && curl -s -L -o  boost_${BOOST_VERSION}.tar.bz2 https://dl.bintray.com/boostorg/release/${BOOST_VERSION_DOT}/source/boost_${BOOST_VERSION}.tar.bz2 \
    && echo "${BOOST_HASH}  boost_${BOOST_VERSION}.tar.bz2" | sha256sum -c \
    && tar -xvf boost_${BOOST_VERSION}.tar.bz2 \
    && cd boost_${BOOST_VERSION} \
    && ./bootstrap.sh \
    && ./b2 --with-chrono --with-date_time --with-filesystem --with-log --with-program_options --with-random --with-regex --with-serialization --with-system --with-test --with-thread threading=multi threadapi=pthread cflags="-fPIC" cxxflags="-fPIC" install
ENV BOOST_ROOT /usr/local/boost_${BOOST_VERSION}

# We build uhd because the Debian package includes a ton of things we don't want, mostly GUI and Python things
# https://files.ettus.com/manual/page_build_guide.html
ADD https://api.github.com/repos/EttusResearch/uhd/git/refs/heads/UHD-3.15.LTS uhd_version.json
RUN git clone https://github.com/EttusResearch/uhd.git && \
    cd uhd/host && \
    git checkout UHD-3.15.LTS && \
    mkdir build && cd build && \
    cmake -DENABLE_PYTHON_API=OFF .. && \
    make -j`nproc` && make test && make install && ldconfig

# https://wiki.gnuradio.org/index.php/InstallingGR#For_GNU_Radio_3.8_or_Earlier
ADD https://api.github.com/repos/gnuradio/gnuradio/git/refs/heads/maint-3.8 gnuradio_version.json
RUN git clone https://github.com/gnuradio/gnuradio.git && \
    cd gnuradio && \
    git checkout maint-3.8 && \
    git submodule update --init --recursive && \
    # Volk must be compiled dynamic for some reason
    cd volk && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 ../ && \
    make -j`nproc` && make test && make install && ldconfig && \
    cd ../../ && \
    mkdir build && cd build && \
    cmake -DENABLE_INTERNAL_VOLK=OFF -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 -DENABLE_TESTING=OFF -DENABLE_PYTHON=OFF -DENABLE_STATIC_LIBS=ON ../ && \
    make -j`nproc` && make test && make install && ldconfig

# BladeRF in Debian is incompatible with gr-osmosdr, so let's build it
# https://github.com/Nuand/bladeRF/wiki/Getting-Started%3A-Linux#Building_bladeRF_libraries_and_tools_from_source
# No need for version check because its a tag but can't download tgz due to submodules
RUN git clone https://github.com/Nuand/bladeRF.git && \
    cd bladeRF/host && \
    git checkout 2021.03 && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DINSTALL_UDEV_RULES=OFF ../ && \
    make -j`nproc` && make install && ldconfig

# https://osmocom.org/projects/gr-osmosdr/wiki
RUN curl -OL https://github.com/osmocom/gr-osmosdr/archive/refs/tags/v0.2.3.tar.gz && \
    tar xvfz v0.2.3.tar.gz && \
    cd gr-osmosdr-0.2.3 && \
    mkdir build && cd build && \
    cmake -DENABLE_RTL=ON -DENABLE_HACKRF=ON -DENABLE_BLADERF=ON -DENABLE_AIRSPY=ON -DENABLE_AIRSPYHF=ON -DENABLE_SOAPY=ON ../ && \
    make -j`nproc` && make test && make install && ldconfig

RUN mkdir /src/trunk-recorder

COPY . /src/trunk-recorder/

RUN cd /src/trunk-recorder && cmake . && make -j`nproc`

FROM debian:buster-slim

COPY --from=deps /src/trunk-recorder/recorder /recorder

USER nobody

# GNURadio requires a place to store some files, can only be set via $HOME env var.
ENV HOME=/tmp

CMD ["/recorder", "--config=/config.json"]

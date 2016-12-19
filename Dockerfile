# docker build -t robotastic/trunk-recorder:latest .

FROM robotastic/docker-gnuradio:latest



COPY . /src/trunk-recorder
RUN cd /src/trunk-recorder && cmake . && make
RUN mkdir /app && cp /src/trunk-recorder/recorder /app

# ffmpeg - http://ffmpeg.org/download.html
#
# From https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu
#
# https://hub.docker.com/r/jrottenberg/ffmpeg/
#
#

WORKDIR     /tmp/workdir


ENV         FFMPEG_VERSION=3.2.2 \
            FDKAAC_VERSION=0.1.4 \
            LAME_VERSION=3.99.5  \
            OGG_VERSION=1.3.2    \
            OPUS_VERSION=1.1.1   \
            THEORA_VERSION=1.1.1 \
            YASM_VERSION=1.3.0   \
            VORBIS_VERSION=1.3.5 \
            VPX_VERSION=1.6.0    \
            XVID_VERSION=1.3.4   \
            X265_VERSION=2.0     \
            X264_VERSION=20160826-2245-stable \
            PKG_CONFIG_PATH=/usr/local/lib/pkgconfig \
            SRC=/usr/local

RUN      buildDeps="autoconf \
                    automake \
                    cmake \
                    curl \
                    bzip2 \
                    g++ \
                    gcc \
                    git \
                    libtool \
                    make \
                    nasm \
                    perl \
                    pkg-config \
                    python \
                    libssl-dev \
                    yasm \
                    zlib1g-dev" && \
        export MAKEFLAGS="-j$(($(nproc) + 1))" && \
        apt-get -yqq update && \
        apt-get install -yq --no-install-recommends ${buildDeps} ca-certificates && \

        DIR=$(mktemp -d) && cd ${DIR} && \
## x264 http://www.videolan.org/developers/x264.html
        curl -sL https://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-${X264_VERSION}.tar.bz2 | \
        tar -jx --strip-components=1 && \
        ./configure --prefix="${SRC}" --bindir="${SRC}/bin" --enable-pic --enable-shared --disable-cli && \
        make && \
        make install && \
        rm -rf ${DIR} && \
        DIR=$(mktemp -d) && cd ${DIR} && \
## x265 http://x265.org/
        curl -sL https://download.videolan.org/pub/videolan/x265/x265_${X265_VERSION}.tar.gz  | \
        tar -zx && \
        cd x265_${X265_VERSION}/build/linux && \
        ./multilib.sh && \
        make -C 8bit install && \
        rm -rf ${DIR} && \
        DIR=$(mktemp -d) && cd ${DIR} && \
## fdk-aac https://github.com/mstorsjo/fdk-aac
        curl -sL https://github.com/mstorsjo/fdk-aac/archive/v${FDKAAC_VERSION}.tar.gz | \
        tar -zx --strip-components=1 && \
        autoreconf -fiv && \
        ./configure --prefix="${SRC}" --disable-static --datadir="${DIR}" && \
        make && \
        make install && \
        make distclean && \
        rm -rf ${DIR} && \
        DIR=$(mktemp -d) && cd ${DIR} && \
## ffmpeg https://ffmpeg.org/
        curl -sL http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz | \
        tar -zx --strip-components=1 && \
        ./configure --prefix="${SRC}" \
        --extra-cflags="-I${SRC}/include" \
        --extra-ldflags="-L${SRC}/lib" \
        --bindir="${SRC}/bin" \
        --disable-doc \
	--disable-static \
	--enable-shared \
	--disable-ffplay \
        --extra-libs=-ldl \
        --enable-version3 \
        --enable-libfdk_aac \
        --enable-libx264 \
        --enable-libx265 \
	--enable-gpl \
        --enable-avresample \
        --enable-postproc \
        --enable-nonfree \
        --disable-debug \
        --enable-small \
        --enable-openssl && \
        make && \
        make install && \
        echo "include /usr/local/lib/" >> /etc/ld.so.conf && \
        ldconfig && \
## cleanup
       cd && \
       apt-get autoremove -y && \
       apt-get clean -y && \
       rm -rf /var/lib/apt/lists && \
       ffmpeg -buildconf

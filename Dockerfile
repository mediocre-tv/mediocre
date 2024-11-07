ARG UBUNTU_VERSION=24.04
FROM ubuntu:$UBUNTU_VERSION AS develop

# use bash instead of sh
SHELL ["/bin/bash", "-c"]

# installation tools
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y wget git unzip

# install build tools
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y build-essential gdb

# install cmake
# check support in Clion > Build, Execution, Deployment > Toolchains before upgrading cmake
ARG CMAKE_VERSION=3.29.5
RUN mkdir -p /local/cmake && cd "$_" \
    && mkdir ./download && mkdir ./install \
    && wget -q -O ./download/cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION\-linux-x86_64.sh \
    && sh ./download/cmake-linux.sh -- --skip-license --prefix=./install
ENV PATH=$PATH:/local/cmake/install/bin

# install ninja
ARG NINJA_VERSION=1.12.1
RUN mkdir -p /local/ninja && cd "$_" \
    && mkdir ./download && mkdir ./install \
    && wget -q -O ./download/source.zip https://github.com/ninja-build/ninja/releases/download/v$NINJA_VERSION/ninja-linux.zip \
    && unzip ./download/source.zip -d ./install
ENV PATH=$PATH:/local/ninja/install

# install grpc and protobuf
# there is some issue with upgrading grpc regarding std::thread
ARG GRPC_VERSION=1.64.3
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y autoconf libtool pkg-config
RUN --mount=type=cache,target=/local/grpc/download \
    --mount=type=cache,target=/local/grpc/build \
    cd /local/grpc/build \
    && mkdir ../install \
    && ( [ ! -d ../download/grpc ] \
        && ( git clone --recurse-submodules -b v$GRPC_VERSION --depth 1 --shallow-submodules https://github.com/grpc/grpc ../download/grpc ) \
        || ( pushd ../download/grpc \
            && ( [[ $(git describe --tags) == v$GRPC_VERSION ]] \
                && echo "Already on correct tag" \
                || ( git fetch origin v$GRPC_VERSION:v$GRPC_VERSION --recurse-submodules --depth 1 \
                    && git switch --detach v$GRPC_VERSION \
                    && git submodule update --single-branch )) \
            && popd )) \
    && cmake -D gRPC_BUILD_TESTS=OFF \
             -D CMAKE_INSTALL_PREFIX=../install \
             -G Ninja \
             -S ../download/grpc \
             -B . \
    && ninja install
ENV CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:/local/grpc/install/lib/cmake
ENV PATH=$PATH:/local/grpc/install/bin

# install opencv
# we should be building staticly, but see https://github.com/opencv/opencv/issues/21447#issuecomment-1013088996
ARG OPENCV_VERSION=4.10.0
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y ffmpeg libavformat-dev libavcodec-dev libswscale-dev
RUN --mount=type=cache,target=/local/opencv/build \
    cd /local/opencv/build \
    && mkdir ../download && mkdir ../install \
    && wget -q -O ../download/source.zip https://github.com/opencv/opencv/archive/$OPENCV_VERSION.zip \
    && unzip ../download/source.zip -d ../download \
    && cmake -D CMAKE_INSTALL_PREFIX=../install \
             -D BUILD_opencv_highgui=OFF \
             -D WITH_PROTOBUF=ON \
             -D BUILD_PROTOBUF=OFF \
             -D WITH_FFMPEG=ON \
             -D BUILD_TESTS=OFF \
             -D BUILD_PERF_TESTS=OFF \
             -G Ninja \
             -S ../download/opencv-$OPENCV_VERSION \
             -B . \
    && ninja install
ENV CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:/local/opencv/install/lib/cmake
ENV PATH=$PATH:/local/opencv/install/bin

# install leptonica
ARG LEPTONICA_VERSION=1.83.1
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y libicu-dev libpango1.0-dev libcairo2-dev libtiff-dev libjpeg-dev
RUN --mount=type=cache,target=/local/leptonica/build \
    cd /local/leptonica/build \
    && mkdir ../download && mkdir ../install \
    && wget -q -O ../download/source.zip https://github.com/DanBloomberg/leptonica/archive/refs/tags/$LEPTONICA_VERSION.zip \
    && unzip ../download/source.zip -d ../download \
    && cmake -D CMAKE_INSTALL_PREFIX=../install \
             -G Ninja \
             -S ../download/leptonica-$LEPTONICA_VERSION \
             -B . \
    && ninja install
ENV CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:/local/leptonica/install/lib/cmake

# install tesseract
ARG TESSERACT_VERSION=5.4.1
RUN --mount=type=cache,target=/local/tesseract/build \
    cd /local/tesseract/build \
    && mkdir ../download && mkdir ../install \
    && wget -q -O ../download/source.zip https://github.com/tesseract-ocr/tesseract/archive/refs/tags/$TESSERACT_VERSION.zip \
    && unzip ../download/source.zip -d ../download \
    && cmake -D CMAKE_INSTALL_PREFIX=../install \
             -G Ninja \
             -S ../download/tesseract-$TESSERACT_VERSION \
             -B . \
    && ninja install
ENV CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:/local/tesseract/install/lib/cmake
ENV PATH=$PATH:/local/tesseract/install/bin

# install tesseract languages
ARG TESSDATA_VERSION=4.1.0
RUN wget -q -P /local/tesseract/install/share/tessdata/ https://github.com/tesseract-ocr/tessdata_best/raw/$TESSDATA_VERSION/eng.traineddata

# install grpc client cli
ARG GRPC_CLIENT_CLI_VERSION=1.21.1
RUN mkdir -p /local/grpc-client-cli && cd "$_" \
    && mkdir ./download && mkdir ./install \
    && wget -q -O ./download/source.tar.gz https://github.com/vadimi/grpc-client-cli/releases/download/v$GRPC_CLIENT_CLI_VERSION/grpc-client-cli_linux_x86_64.tar.gz \
    && tar -xzf ./download/source.tar.gz -C ./install
ENV PATH=$PATH:/local/grpc-client-cli/install

# install grpc web proxy
ARG GRPC_WEB_PROXY_VERSION=0.15.0
RUN mkdir -p /local/grpcwebproxy && cd "$_" \
    && mkdir ./download && mkdir ./install \
    && wget -q -O ./download/source.zip https://github.com/improbable-eng/grpc-web/releases/download/v$GRPC_WEB_PROXY_VERSION/grpcwebproxy-v$GRPC_WEB_PROXY_VERSION\-linux-x86_64.zip \
    && unzip ./download/source.zip -d ./download \
    && cp ./download/dist/grpcwebproxy-v$GRPC_WEB_PROXY_VERSION\-linux-x86_64 ./install/grpcwebproxy
ENV PATH=$PATH:/local/grpcwebproxy/bin

# install apprise
ARG APPRISE_VERSION=1.9.0
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y python3-pip
RUN pip install apprise==$APPRISE_VERSION --break-system-packages


FROM develop AS build

# install mediocre
RUN --mount=type=bind,source=mediocre,target=/local/mediocre/source/mediocre \
    --mount=type=bind,source=grpc,target=/local/mediocre/source/grpc \
    --mount=type=bind,source=CMakeLists.txt,target=/local/mediocre/source/CMakeLists.txt \
    --mount=type=cache,target=/local/mediocre/build \
    cd /local/mediocre/build \
    && mkdir ../install \
    && cmake -D CMAKE_INSTALL_PREFIX=../install \
             -G Ninja \
             -S ../source \
             -B . \
    && ninja install
ENV PATH=$PATH:/local/mediocre/install/bin


FROM ubuntu:$UBUNTU_VERSION AS release

# install required shared libraries
# opencv also builds these, can we use those instead?
RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update \
    && apt-get install -y libicu-dev libpango1.0-dev libcairo2-dev libtiff-dev libjpeg-dev \
                          ffmpeg libavformat-dev libavcodec-dev libswscale-dev

# copy required files/settings from build stage
# TESSDATA_PREFIX is required for tesseract at runtime
ENV TESSDATA_PREFIX=/local/tesseract/install/share/tessdata/
COPY --from=build $TESSDATA_PREFIX $TESSDATA_PREFIX
COPY --from=build /local/grpc-client-cli/install /app/bin
COPY --from=build /local/grpcwebproxy/install /app/bin
# see "why can't you just be normal?" meme for why this is necessary
COPY --from=build /local/opencv/install/lib/libopencv_imgproc.so.* /local/opencv/install/lib/
COPY --from=build /local/mediocre/install /app
ENV PATH=$PATH:/app/bin

# setup the proxy script
COPY scripts/run/proxy.sh /scripts/run-proxy.sh
RUN ["chmod", "+x", "/scripts/run-proxy.sh"]
ENV TLS_CERT_PATH=${TLS_CERT_PATH:-/certificates/cert.pem}
ENV TLS_KEY_PATH=${TLS_KEY_PATH:-/certificates/key.pem}

# run mediocre
ENTRYPOINT ["/bin/bash", "-c", "mediocre & /scripts/run-proxy.sh"]
EXPOSE 8080
EXPOSE 8443

# wish to use grpcwebproxy for healthchecks, since that would be more accurate given we only expose the proxy ports
# however, it seems to think that the grpc server is healthy even when it is not serving, or is offline
HEALTHCHECK --start-period=30s --start-interval=1s --interval=30s --timeout=3s \
  CMD grpc-client-cli health 0.0.0.0:8081
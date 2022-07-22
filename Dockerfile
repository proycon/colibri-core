FROM alpine:latest
LABEL org.opencontainers.image.authors="Maarten van Gompel <proycon@anaproy.nl>"
LABEL description="Colibri Core is software to quickly and efficiently count and extract patterns"

RUN mkdir -p /usr/src/colibri-core
COPY . /usr/src/colibri-core

RUN PACKAGES="libbz2 libstdc++" &&\
    BUILD_PACKAGES="build-base autoconf-archive autoconf automake libtool bzip2-dev git" &&\
    apk add $PACKAGES $BUILD_PACKAGES &&\ 
    cd /usr/src/colibri-core && sh ./bootstrap && ./configure && make && make install &&\
    apk del $BUILD_PACKAGES && rm -Rf /usr/src

WORKDIR /

ENTRYPOINT [ "sh" ]

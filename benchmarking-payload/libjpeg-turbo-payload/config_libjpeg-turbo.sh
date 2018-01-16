#! /bin/bash

# Init source code
SRC_DIR="$(pwd)/libjpeg-turbo-src"

DEFAULT_CFLAGS="-m32"
DEFAULT_LDFLAGS="-m32"
CC=gcc
CXX=g++

init_target() {
	if ! [ -d $SRC_DIR ]; then
		wget https://github.com/libjpeg-turbo/libjpeg-turbo/archive/1.5.1.tar.gz
		tar xvf 1.5.1.tar.gz
		mv libjpeg-turbo-1.5.1 $SRC_DIR
		cd $SRC_DIR
		autoreconf -fiv
		cd -
	fi
}

LIBTURBOJPEG_FLAGS="--host i686-pc-linux-gnu"
LIBTURBOJPEG_CFLAGS="-O3 -m32"
LIBTURBOJPEG_LDFLAGS="-m32"

build_target() {
	cd $SRC_DIR
	CC="$CC" CFLAGS="$DEFAULT_CFLAGS $LIBTURBOJPEG_CFLAGS" \
	LDFLAGS="$DEFAULT_LDFLAGS $LIBTURBOJPEG_LDFLAGS" \
	$SRC_DIR/configure $LIBTURBOJPEG_FLAGS

	make
	cd -
}

init_target
build_target

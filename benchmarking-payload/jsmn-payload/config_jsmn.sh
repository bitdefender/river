#! /bin/bash

# Init http-parser source code
SRC_DIR="$(pwd)/libjsmn-src"

DEFAULT_CFLAGS="-m32"
DEFAULT_LDFLAGS="-m32"
CC=gcc
CXX=g++

# Build http-parser with the given `name` and flags.
build_target() {

	if ! [ -d $SRC_DIR ]; then
		git clone https://github.com/zserge/jsmn.git $SRC_DIR
	fi

	cd $SRC_DIR
	make clean
	CC="$CC" CXX="$CXX" CXXFLAGS="$DEFAULT_CFLAGS" \
		CFLAGS="$DEFAULT_CFLAGS" LDFLAGS="$DEFAULT_LDFLAGS" \
		make all

	cd -
}

build_target

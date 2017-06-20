#! /bin/bash

# Init source code
SRC_DIR="$(pwd)/gumbo-src"
BUILD_DIR="$(pwd)/build"

DEFAULT_CFLAGS="-m32"
DEFAULT_LDFLAGS="-m32"
CC=gcc
CXX=g++

init_target() {
	if ! [ -d $SRC_DIR ]; then
		git clone https://github.com/google/gumbo-parser.git $SRC_DIR
		cd $SRC_DIR
		./autogen.sh
		cd -
	fi
}


# Build http-parser with the given `name` and flags.
build_target() {
	if [ ! -d $BUILD_DIR ]; then
		mkdir -p $BUILD_DIR
	fi
	cd $BUILD_DIR
	CC="$CC" CXX="$CXX" CXXFLAGS="$DEFAULT_CFLAGS" CFLAGS="$DEFAULT_CFLAGS" \
		LDFLAGS="$DEFAULT_LDFLAGS" $SRC_DIR/configure

	make
	cd -
}

init_target
build_target

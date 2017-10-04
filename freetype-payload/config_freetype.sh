#! /bin/bash

# Init source code
SRC_DIR="$(pwd)/freetype-src"

DEFAULT_CFLAGS="-m32"
DEFAULT_LDFLAGS="-m32"
CC=gcc

init_target() {
	if ! [ -d $SRC_DIR ]; then
		wget http://download.savannah.gnu.org/releases/freetype/freetype-2.8.tar.gz
		tar xvf freetype-2.8.tar.gz
		mv freetype-2.8 $SRC_DIR
		cd $SRC_DIR
		./autogen.sh
		cd -
	fi
}


# Build http-parser with the given `name` and flags.
build_target() {
	cd $SRC_DIR
	CC="$CC" CFLAGS="$DEFAULT_CFLAGS" LDFLAGS="$DEFAULT_LDFLAGS" \
	   	LD="ld -m elf_i386" ./configure
	make
	make library
	cd -
}

init_target
build_target

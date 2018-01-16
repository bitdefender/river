#! /bin/bash

ARGS="--enable-option-checking --disable-shared --disable-ipv6 --without-c14n --without-catalog
--without-debug --without-docbook --without-ftp --without-http --without-legacy
--without-output --without-pattern --without-push --without-python
--without-reader --without-readline --without-regexps --without-sax1 --without-schemas
--without-schematron --without-threads --without-valid --without-writer --without-xinclude
--without-xpath --without-xptr --with-zlib=no --with-lzma=no"

NO_SSE="-mno-mmx -mno-sse -march=i386"

init_target() {
	if ! [ -d libxml2-src ]; then
		git clone git://git.gnome.org/libxml2 libxml2-src
		(
		cd libxml2-src
		autoreconf -fiv
		)
	fi
}

build_target() {

	mkdir -p build
	cd build
	CC="$CC" CXX="$CXX" CFLAGS="-m32 $NO_SSE" LDFLAGS="-m32" \
		../libxml2-src/configure $ARGS
	make -j $N_CORES libxml2.la include/libxml/xmlversion.h
	cd ..
}

init_target
build_target

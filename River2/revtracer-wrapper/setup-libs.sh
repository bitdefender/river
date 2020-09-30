#! /bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -rf $DIR/lib
mkdir $DIR/lib

ln --symbolic -T `find /lib -name libc.so.6 -path *i386*` $DIR//lib/libc.so
ln --symbolic -T `find /lib -name libpthread.so.0 -path *i386*` $DIR/lib/libpthread.so
ln --symbolic -T `find /lib -name librt.so.1 -path *i386*` $DIR/lib/librt.so
ln --symbolic -T `find /lib -name ld-linux.so.2 -path *i386*` $DIR/lib/ld-linux.so.2

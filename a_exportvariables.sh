TOOLS_DIR=$1

printf "${TOOLS_DIR} \n"

export RIVER_NATIVE_LIBS=/usr/local/lib
export LIBC_PATH=/lib32/libc.so.6
export LIBPTHREAD_PATH=/lib32/libpthread.so.0
export Z3_ROOT=~/$TOOLS_DIR/river/z3
export LD_LIBRARY_PATH=/usr/local/lib/

printf $Z3_ROOT


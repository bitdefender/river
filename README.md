# River 

- The instalation process is for Ubuntu 16.04.5 LTS xenial version
- Also, note that the process is described for all of our tools not just RIVER.

## Steps setup 

1. Install prerequisits:

```
sudo apt-get install -y git cmake build-essential gcc-multilib g++-multilib mingw-w64 python  python-pip unzip autoconf libtool nasm
sudo apt --reinstall install libc6 libc6-dev-i386
```

2. Clone river, simple.tracer and river.format from [AGAPIA](https://github.com/AGAPIA)

create a folder like /home/YOURUSERNAME/testtools

cd there

then clone the repositories
```
git clone https://github.com/AGAPIA/river.format
git clone https://github.com/AGAPIA/river
git clone https://github.com/AGAPIA/simpletracer
```

4. Install river.

```
cd river/
ln -s /home/YOURUSERNAME/testtools/river.format ./  # Note that river.format is shared between river and simple/distributed tracer. This command will create a symbolic link such that river thinks that river.format is a subfolder
```

```
cmake CMakeLists.txt
make
sudo make install
``` 

5. Create Z3 symbolic link to /usr/local/lib such that tracer app cand find it

```
cd /usr/local/lib/
sudo su
ln -s /home/YOURUSERNAME/testtools/river/z3/bin/libz3.* ./
```

6. Set variables and create links for simpletracer installation.
```
RIVER_NATIVE_LIBS=/usr/local/lib
LIBC_PATH=/lib32/libc.so.6
LIBPTHREAD_PATH=/lib32/libpthread.so.0
sudo ln -s -T $LIBC_PATH $RIVER_NATIVE_LIBS/libc.so
sudo ln -s -T $LIBPTHREAD_PATH $RIVER_NATIVE_LIBS/libpthread.so
export Z3_ROOT_PATH=/home/YOURUSERNAME/testtools/river/z3
export Z3_ROOT=/home/YOURUSERNAME/testtools/river/z3
export LD_LIBRARY_PATH=/usr/local/lib/
```

Note: In case that there is no libpthread.so.0 or libc.so.6 in lib32 do the following steps:
```
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386
sudo ln --symbolic -T `find /lib -name libpthread.so.0 -path *i386*` libpthread.so.0
```
Then, the same command but for libc.so.6


7. Create symbolic link for river.format inside simpletracer.
```
cd simpletracer/
ln -s ../river.format/ ./
```

8. Install simpletracer.
```
cmake CMakeLists.txt
make
sudo make install
```

9. Test simpletracer.
```
python -c 'print "B" * 100' | river.tracer -p libjsmn-parser.so
``` 

This command should create two files:
- execution.log
- trace.simple.out

If you add --annotated or --z3 you'll get tainted index logs or z3 conditions desc too

Note: 
If you don't already have `libjsmn-parser.so`: 
```
cd river/benchmarking-payload/jsmn-payload/

cmake CMakeLists.txt 
make 
sudo make install 
```
Then the `libjsmn-parser.so` can be found in `/usr/local/lib`.


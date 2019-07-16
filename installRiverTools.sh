set -e


# This script will download, build and install River

# Tools dir is the name where the download will go (inside ~/ folder)
TOOLSDIR=$1 # can be : newinstall OR cleanbuild OR build
BUILD_TYPE=$2 # can be Debug or Release

BLUE='\033[0;34m'
NC='\033[0m' # No Color

FULL_TOOLSDIR=~/$TOOLSDIR

#if false; then
if [ -d "$FULL_TOOLSDIR" ]; then
  printf "ERROR: Folder ${FULL_TOOLSDIR} already exists ! Please delete it yourself or backup everything"
  exit 1
fi

printf "${BLUE}Step 1: installing tools\n${NC}"

sudo apt-get install -y git cmake build-essential gcc-multilib g++-multilib mingw-w64 python  python-pip unzip autoconf libtool nasm
sudo apt --reinstall install libc6 libc6-dev-i386

cd ~/
if [ -d "$FULL_TOOLSDIR" ]; then rm -f -r $FULL_TOOLSDIR; fi
mkdir $FULL_TOOLSDIR
cd $FULL_TOOLSDIR

git clone https://github.com/AGAPIA/river.format
git clone https://github.com/AGAPIA/river
git clone https://github.com/AGAPIA/simpletracer

#fi # If false

printf "${BLUE}Step 2: Setting env variables and make sys libs point to River's ${NC}"
cd $FULL_TOOLSDIR/river
#sudo ./a_exportvariables.sh $TOOLSDIR
export RIVER_NATIVE_LIBS=/usr/local/lib  # Somehow the above command doesn't store correctly so we duplicate them..they are already duplicated in buildtools.sh
export LIBC_PATH=/lib32/libc.so.6
export LIBPTHREAD_PATH=/lib32/libpthread.so.0
export Z3_ROOT=~/z3
export LD_LIBRARY_PATH=/usr/local/lib/


# Write variables in bashrc
echo 'RIVER_NATIVE_LIBS=/usr/local/lib' >> ~/.bashrc 
echo 'LIBC_PATH=/lib32/libc.so.6' >> ~/.bashrc 
echo 'LIBPTHREAD_PATH=/lib32/libpthread.so.0' >> ~/.bashrc 
echo 'export Z3_ROOT_PATH=~/z3' >> ~/.bashrc 
echo 'export Z3_ROOT=~/z3' >> ~/.bashrc 
echo 'export LD_LIBRARY_PATH=/usr/local/lib/' >> ~/.bashrc 


printf "Variable is $RIVER_NATIVE_LIBS"
sudo ln -s -f -T $LIBC_PATH $RIVER_NATIVE_LIBS/libc.so
sudo ln -s -f -T $LIBPTHREAD_PATH $RIVER_NATIVE_LIBS/libpthread.so


if [ "$BUILD_TYPE" = "Debug"  ]
then
	printf "Producing a Debug build\n"
else
	printf "Producing a Release build\n"
	BUILD_TYPE = "Release"
fi

sudo ./a_buildtools.sh $BUILD_TYPE $TOOLSDIR  clean





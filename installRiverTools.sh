set -e

CLEAN_BUILD=$1
WRITE_IN_BASHRC=$2

TOOLSDIR="testtools"
BLUE='\033[0;34m'
NC='\033[0m' # No Color


if [ "$CLEAN_BUILD" = "clean"  ]; then
printf "${BLUE}Step 1: installing tools\n${NC}"

sudo apt-get install -y git cmake build-essential gcc-multilib g++-multilib mingw-w64 python  python-pip unzip autoconf libtool nasm
sudo apt --reinstall install libc6 libc6-dev-i386

cd ~/
if [ -d "$TOOLSDIR" ]; then rm -f -r $TOOLSDIR; fi
mkdir $TOOLSDIR
cd $TOOLSDIR

git clone https://github.com/AGAPIA/river.format
git clone https://github.com/AGAPIA/river
git clone https://github.com/AGAPIA/simpletracer

printf "${BLUE}Step 2: Build and install River\n${NC}"
cd ~/$TOOLSDIR/river/
rm -f river.format # delete the symbolic link if there is already there from github..
ln -s ~/$TOOLSDIR/river.format ./  # Note that river.format is shared between river and simple/distributed tracer. This command will create a symbolic link such that river thinks that river.format is a subfolder

cmake CMakeLists.txt
make
sudo make install

printf "${BLUE}Step 3: Setting Z3: ${NC}"
cd /usr/local/lib/
sudo rm -f ./libz3.*
sudo ln -s ~/$TOOLSDIR/river/z3/bin/libz3.* ./
cd -

printf "${BLUE}Step 4: Setting env variables and make sys libs point to River's ${NC}"
RIVER_NATIVE_LIBS=/usr/local/lib
LIBC_PATH=/lib32/libc.so.6
LIBPTHREAD_PATH=/lib32/libpthread.so.0
export Z3_ROOT_PATH=~/$TOOLSDIR/river/z3
export Z3_ROOT=/home/$TOOLSDIR/testtools/river/z3
export LD_LIBRARY_PATH=/usr/local/lib/

sudo ln -s -f -T $LIBC_PATH $RIVER_NATIVE_LIBS/libc.so
sudo ln -s -f -T $LIBPTHREAD_PATH $RIVER_NATIVE_LIBS/libpthread.so

printf "${BLUE}Step 5: Installing simpletracer ${NC}"
cd ~/$TOOLSDIR/simpletracer/
rm -f river.format # remove the symbolic link if it came from github..
ln -s ../river.format/ ./
cmake CMakeLists.txt
make
sudo make install

fi

if [ "$WRITE_IN_BASHRC" = "bashrc"  ]; then
echo 'RIVER_NATIVE_LIBS=/usr/local/lib' >> ~/.bashrc 
echo 'LIBC_PATH=/lib32/libc.so.6' >> ~/.bashrc 
echo 'LIBPTHREAD_PATH=/lib32/libpthread.so.0' >> ~/.bashrc 
echo 'export Z3_ROOT_PATH=~/'$TOOLSDIR'/river/z3' >> ~/.bashrc 
echo 'export Z3_ROOT=~/'$TOOLSDIR'/river/z3' >> ~/.bashrc 
echo 'export LD_LIBRARY_PATH=/usr/local/lib/' >> ~/.bashrc 
fi





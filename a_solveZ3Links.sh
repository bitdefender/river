# Setting Z3
BUILD_TYPE=$1
TOOLSDIR=$2

printf "## Setting Z3 links ##\n"
printf "Build type is ${BUILD_TYPE}\n"
printf "Tools dir is ${TOOLSDIR}\n"

# Download then unzip
cd ~/$TOOLSDIR/river/
#unzip z3.zip -d ./

cd /usr/local/lib/
sudo rm -f ./libz3.so

if [ "$BUILD_TYPE" = "Debug"  ]
then
	sudo ln -s ~/z3/Debug/lib/libz3.so ./
else
	sudo ln -s ~/z3/Release/lib/libz3.so ./
fi

cd -


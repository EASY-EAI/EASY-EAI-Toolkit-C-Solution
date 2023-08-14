#!/bin/sh

set -e

SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
cd $SHELL_FOLDER

CUR_DIR_NAME=`basename "$SHELL_FOLDER"`
warring() {
	echo "DESCRIPTION"
	echo "EASYAI-1109_1126 Solution Project."
	echo " "
	echo "./build.sh       : build solution && copy app file"
	echo "./build.sh cpres : build solution && copy all resource files(include app)"
	echo "./build.sh clear : clear all compiled files(just preserve source code)"
	echo " "
}

if [ "$1" = "clear" ]; then
	rm -rf build
	rm -rf Release
	exit 0
fi

# build this project
rm -rf build
mkdir build
cd build
cmake ..
make -j24

# make Release files
mkdir -p "../Release" && cp $CUR_DIR_NAME "../Release"
cp ../config/rtspClient.ini ../Release
chmod 777 ../Release -R

# install files to Board env
if [ "$1" = "cpres" ]; then
        cp ../Release/* $SYSROOT/userdata/Solu
else
        cp ../Release/$CUR_DIR_NAME $SYSROOT/userdata/Solu
#elif [ $# -eq 0 ]; then
#	cp ../Release/$CUR_DIR_NAME $SYSROOT/userdata/Solu
#else
#	warring
fi

exit 0

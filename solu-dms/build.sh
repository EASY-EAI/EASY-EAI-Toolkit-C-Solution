#!/bin/sh

set -e

SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
cd $SHELL_FOLDER

CUR_DIR_NAME=`basename "$SHELL_FOLDER"`

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
cp ../images/*.jpg ../Release
chmod 777 ../Release -R

# install files to Board env
if [ "$1" = "cpres" ]; then
        cp ../Release/* $SYSROOT/userdata/Solu
else
        cp ../Release/$CUR_DIR_NAME $SYSROOT/userdata/Solu
fi

exit 0

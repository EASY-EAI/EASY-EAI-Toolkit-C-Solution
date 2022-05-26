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

rm -rf build
mkdir build
cd build
cmake ..
make -j24

mkdir -p "../Release" && cp $CUR_DIR_NAME "../Release"
cp ../config/rtspClient.ini ../Release

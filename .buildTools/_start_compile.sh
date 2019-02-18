#!/usr/bin/env bash

RootPath="$(
	cd "$(dirname "$0")"
	pwd -P
)/..";
cd $RootPath

BuildDir="build-$(uname)"

if [ "$1" = "--get-build-directory" ]; then
	echo $BuildDir
	exit 0
fi

if [ "$(uname)" = "Linux" ]; then
	ConanConfigFile="linux.txt";
	CMakeType="Unix Makefiles"
else
	ConanConfigFile="windows.txt";
	CMakeType="Visual Studio 15 Win64"
fi

if [ ! -d $BuildDir ]; then
	mkdir $BuildDir
fi
(
	cd $BuildDir
	conan remote add bincrafter https://api.bintray.com/conan/bincrafters/public-conan 2> /dev/null

	conan install .. --build=missing --settings build_type=$1 --profile ../.conan_platform_settings/$ConanConfigFile
	cmake .. -G "$CMakeType" -DCMAKE_BUILD_TYPE=$1

	cmake --build . --config $@
) && ( echo "OK"; exit 0 ) || ( echo "KO"; exit 1 );
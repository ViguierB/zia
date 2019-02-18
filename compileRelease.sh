#!/usr/bin/env sh

RootPath="$(
	cd "$(dirname "$0")"
	pwd -P
)/.";
cd $RootPath

echo "Release" | ./.buildTools/_select_compile.sh $@
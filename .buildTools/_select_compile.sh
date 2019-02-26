#!/usr/bin/env bash

RootPath="$(
	cd "$(dirname "$0")"
	pwd -P
)/..";
cd $RootPath

buildType=""
read buildType

echo "Building $buildType..."

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
  echo "usage: $0 [--nodocker {...}]"
  echo " --nodocker: don't use docker (Windows)"
  echo " ...: cmake -build config=$buildType __VA_ARGS__"
  exit 0
elif [ "$1" = "--nodocker" ]; then
	shift
  ./.buildTools/_start_compile.sh $buildType $@
else
	cmd=$(echo "export CONAN_USER_HOME='$(pwd)/.cache'; $(pwd)/.buildTools/_start_compile.sh $buildType $@")

  docker run \
	-v $(pwd):$(pwd) \
	-t \
		epitechcontent/epitest-docker \
			/bin/bash -c "$cmd"
fi
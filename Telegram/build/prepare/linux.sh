#!/bin/bash

# Загружаем переменные
source ../../../config.env

set -e
FullExecPath=$PWD
pushd `dirname $0` > /dev/null
FullScriptPath=`pwd`
popd > /dev/null


cd $FullScriptPath/../docker/centos_env
poetry install
poetry run gen_dockerfile | DOCKER_BUILDKIT=1 docker build \
	--build-arg CPUS="$CPUS" \
	--build-arg MEMORY="$MEMORY"g \
	-t tdesktop:centos_env -
cd $FullExecPath

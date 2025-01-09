#!/bin/bash

# Переменные
YOUR_API_ID="23244845"
YOUR_API_HASH="47b3b70ac69586b8fd2b55ec2e751aa4"

docker run --rm --cpus=7 --memory=20g --gpus all -it \
    -v "$PWD:/usr/src/tdesktop" \
    tdesktop:centos_env \
    /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID="$YOUR_API_ID" \
    -D TDESKTOP_API_HASH="$YOUR_API_HASH"

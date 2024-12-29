FROM ubuntu:jammy

RUN apt-get update
RUN apt-get -y --no-install-recommends install \
    build-essential cmake libboost-program-options-dev libboost-fiber-dev \
    libfmt-dev ronn clang-13 clang-14 \
    libgirepository1.0-dev libgstreamer1.0-dev \
    libgtk-3-dev gir1.2-gtk-3.0 \
    qtbase5-dev
RUN apt-get clean

# sigh ... patch a buggy header
# (only clang seems to complain ... but we need that to work too)
RUN sed -e 's/path::begin() const$/path::begin() const noexcept/' \
        -e 's/path::end() const$/path::end() const noexcept/' \
    -i /usr/include/c++/11/experimental/bits/fs_path.h

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -ms /bin/bash user

USER user
WORKDIR /home/user

ENV LANG C.UTF-8

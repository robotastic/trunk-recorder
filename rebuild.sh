#!/bin/sh
set -e
git pull
mkdir -p build
cd build
rm -rf *
cmake ../
make
sudo cp -i recorder /usr/local/bin/trunk-recorder

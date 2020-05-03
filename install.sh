#! /bin/sh

set -e

# trunk-recorder install script for debian based systems
# including ubuntu 18.04/20.04 and raspbian

if [ ! -d trunk-recorder/recorders ]; then
	echo ====== ERROR: trunk-recorder top level directories not found.
	echo ====== You must change to the trunk-recorder top level directory
	echo ====== before running this script.
	exit
fi

sudo apt-get update
sudo apt-get install gnuradio gnuradio-dev gr-osmosdr libhackrf-dev libuhd-dev libgmp-dev
sudo apt-get install git cmake build-essential libboost-all-dev libusb-1.0-0.dev libssl-dev libcurl4-openssl-dev liborc-0.4-dev

mkdir build
cd build
cmake ../
make
sudo cp -i recorder /usr/local/bin/trunk-recorder

cd ..

if [ ! -f /etc/modprobe.d/blacklist-rtl.conf ]; then
	echo ====== Installing blacklist-rtl.conf for selecting the correct RTL-SDR drivers
	echo ====== Please reboot before running trunk-recorder.
	sudo install -m 0644 ./blacklist-rtl.conf /etc/modprobe.d/
fi

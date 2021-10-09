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

pre_reqs() {
    PKG_LIST="gnuradio gnuradio-dev gr-osmosdr libhackrf-dev libuhd-dev libgmp-dev"
    PKG_LIST="$PKG_LIST fdkaac sox"
    PKG_LIST="$PKG_LIST git cmake build-essential libboost-all-dev libusb-1.0-0.dev libssl-dev libcurl4-openssl-dev liborc-0.4-dev"

    ISSUE=$(cat /etc/issue)
    case "$ISSUE" in
	"") echo "Well that's weird. You don't have an /etc/issue" ;;
	"Ubuntu 20.04*") PKG_LIST="$PKG_LIST pkg-config libpthread-stubs0-dev" ;;
    esac

    sudo apt-get update
    sudo apt-get install $PKG_LIST
}

freshen_repo() {
    git pull
    if [ -d build ]; then
	rm -rf build
    fi
}

do_build() {
    mkdir build
    cd build
    cmake ../
    make
    cd ..
}

do_install() {
    cd build
    sudo make install
    cd ..
}

blacklist() {
    if [ ! -f /etc/modprobe.d/blacklist-rtl.conf ]; then
	echo ====== Installing blacklist-rtl.conf for selecting the correct RTL-SDR drivers
	echo ====== Please reboot before running trunk-recorder.
	sudo install -m 0644 ./blacklist-rtl.conf /etc/modprobe.d/
    fi
}

rebuild_steps() {
    freshen_repo
    do_build
    do_install
}

install_steps() {
    pre_reqs
    rebuild_steps
    blacklist
}

case "$0" in
    *rebuild*) rebuild_steps ;;
    *) install_steps ;;
esac

# Linux Installs

This page covers installing Trunk Recorder on a Linux box. I test everything on Ubuntu, but other flavors of Linux are supported. Instructions are also included for Arch Linux.

## Install Prerequistes
To get started, install all of the required packages. Instructions for different versions are below:

### Ubuntu 21.04

```bash
sudo   apt-get install -y apt-transport-https build-essential ca-certificates fdkaac git gnupg gnuradio gnuradio-dev gr-osmosdr libuhd3.15.0 libuhd-dev libboost-all-dev libcurl4-openssl-dev libgmp-dev libhackrf-dev liborc-0.4-dev libpthread-stubs0-dev libssl-dev libuhd-dev libusb-dev pkg-config software-properties-common cmake sox
```

If you are using a HackRF:

```bash
sudo apt install -y hackrf libhackrf-dev libhackrf0
```


### Ubuntu 20.x

```bash
sudo apt install -y gr-osmosdr osmo-sdr libosmosdr0 libosmosdr-dev libuhd3.15.0 libuhd-dev gnuradio-dev libgnuradio-uhd3.8.1 libgnuradio-osmosdr0.2.0 gcc cpp cmake make build-essential libboost-all-dev  libusb-dev fdkaac sox openssl libssl-dev curl libcurl4 libcurl4-openssl-dev pkg-config liborc-0.4-dev git
```

If you are using a HackRF:

```bash
sudo apt install -y hackrf libhackrf-dev libhackrf0
```

### Older Ubuntu Verions...

These instructions should work on Ubuntu 16.x to 17.x, including Debian 9 and 10. 

- For **Ubuntu 18.04** add bionic universe before updating the available packages list

```bash
sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu bionic universe"
sudo apt-get update
```

Ubuntu 
```bash
sudo apt install gnuradio gnuradio-dev libuhd-dev libgnuradio-uhd3.7.11
```
Debian 
```bash
sudo apt install gnuradio gnuradio-dev libuhd-dev libgnuradio-uhd3.7.13
```

For all SDR based devices...
```bash
sudo apt install gr-osmosdr libosmosdr0
```

If using HackRF or one of its derivatives...
```bash
sudo apt install hackrf libhackrf0 libhackrf-dev
```

```bash
sudo apt install git cmake make build-essential libboost-all-dev libusb-1.0-0.dev libaacs0 libcppunit-dev libcppunit-1.14-0 libssl-dev openssl curl fdkaac sox libcurl3-gnutls libcurl4 libcurl4-openssl-dev
```


### Arch Linux

Make sure your package lists are up to date:
```bash
sudo pacman -Syy
```
It is suggested to make sure your installed packages are up to date and to review the [Arch Linux documentation regarding upgrades](https://wiki.archlinux.org/index.php/System_maintenance#Upgrading_the_system):
```bash
sudo pacman -Syu
```
Most systems will already have `base-devel` group installed, if yours does not:
```bash
sudo pacman -S base-devel
```
Install the packages required to build Trunk Recorder:
```bash
sudo pacman -S cmake git boost gnuradio gnuradio-osmosdr libuhd fdkaac sox
```

## Building Trunk Recorder

In order to keep your copy of the Trunk Recorder source code free of build artifacts created by the build process, it is suggested to create a separate "out-of-tree" build directory. We will use `trunk-build` as our build directory.

Assuming you are in the desired directory to place both trunk-recorder and trunk-build folders to, perform the following...

```bash
mkdir trunk-build
git clone https://github.com/robotastic/trunk-recorder.git
cd trunk-build
cmake ../trunk-recorder
make
sudo make install
```
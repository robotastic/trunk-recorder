# Raspberry Pi

Smaller radio systems can be covered using a Raspberry Pi. If you are interested in doing this, you should really get a Pi 4. It maybe possible to get things running on an older Pi, but you often get unexpect behavior and errors. A Pi 4 can handle 3-4 simulatanious recordings. Make sure you have a good power supply. Also pay attention to heat. If the Pi gets too hot, it will slow down. A good case or fan can help keep it going full tilt.

## RaspberryOS (aka Raspbian)

### Prerequisites
This page assumes the following.
  - You are using a Raspberry Pi 3B+ or 4, anything else probably can't keep up.
  - You have just downloaded the latest version of [Raspbian Buster](https://www.raspberrypi.org/downloads/raspbian/).
  - You have already installed it on the SD Card with something like [etcher](https://etcher.io/).
  - You have setup network access to the device by Ethernet or WiFi.
    - Simply plugging in an Ethernet cable will give you network access in most cases.
    - If you only have WiFi network availability.
      - _You can do the following as many times as needed for multiple networks_
      - (From the console, aka plugged into a TV with a keyboard attached.)
      - `sudo raspi-config`.
      - Select **2 Network Options** and press <kbd>Enter</kbd>.
      - Select **N2 Wi-fi** and press <kbd>Enter</kbd>.
      - **Please enter SSID** and press <kbd>Enter</kbd>.
      - **Please enter passphrase. Leave it empty if none.** and press <kbd>Enter</kbd>.
      - Press <kbd>></kbd> and <kbd>></kbd> again to highlight **Finish** and press <kbd>Enter</kbd>.
      - Reboot the Pi with `sudo reboot`.
  - You are at the console or have secured, enabled and established an SSH session to the device.
    - You can secure your device by changing the default password for pi.
      - `sudo raspi-config`
      - Select **1 Change User Password** and press <kbd>Enter</kbd>.
      - You will now be asked to enter a new password for the pi user and press <kbd>Enter</kbd>.
    - Enabled SSH
      - `sudo raspi-config`
      - Select **5 Interfacing Options** and press <kbd>Enter</kbd>.
      - Select **P2 SSH** and press <kbd>Enter</kbd>.
      - Select **Yes** to the question **Would you like the SSH server to be enabled?** and press <kbd>Enter</kbd>.

### Setup
- Add the Debian Multimedia source and include non-free libraries, like **fdkaac**. Edit the sources.list file: `sudo nano /etc/apt/sources.list`
- and add this line to the end:
```
deb http://www.deb-multimedia.org/ buster main non-free
```
- Download the keys for the apt source and install them:
```bash
wget https://www.deb-multimedia.org/pool/main/d/deb-multimedia-keyring/deb-multimedia-keyring_2016.8.1_all.deb
sudo dpkg -i deb-multimedia-keyring_2016.8.1_all.deb
```
- Update the OS:
```
sudo apt update
sudo apt upgrade
```
- Add all of the libraries needed:
```bash
sudo apt -y install libssl-dev openssl curl fdkaac sox libcurl3-gnutls libcurl4 libcurl4-openssl-dev gnuradio gnuradio-dev gr-osmosdr libhackrf-dev libuhd-dev 
sudo apt -y install git cmake make build-essential libboost-all-dev libusb-1.0-0.dev 
```
- Get the latest version of Trunk Recorder
```
git clone https://github.com/robotastic/trunk-recorder.git ~/trunk-recorder/
```
### Build
```bash
mkdir ~/trunk-build
cd ~/trunk-build
cmake ../trunk-recorder
make -j `nproc`
sudo make install
```

Note:  If the Pi hangs during the `make -j 'nproc'` command, try `make` instead (it may take longer but may also prevent locking up the Pi due to all processor cores being 100% in use)

### Profile
(It takes about 15 minutes for this section.)

Run the command `volk_profile` to ensure that [VOLK (Vector-Optimized Library of Kernels)](https://wiki.gnuradio.org/index.php/Volk) uses the best [SIMD (Single instruction, multiple data)](https://en.wikipedia.org/wiki/SIMD) architecture for your processor.

## Configuration
[Configure the system](https://github.com/robotastic/trunk-recorder#configure).
## Run
`./recorder`

***
## Ubuntu 21.04 (64-bit support!)

## Prerequisites
This page assumes the following.
  - You are using a Raspberry Pi 3B+ or 4, anything else probably can't keep up.
  - You have just downloaded the latest working version (18.0.4) of [Ubuntu for ARM64](http://cdimage.ubuntu.com/releases/bionic/release/ubuntu-18.04.4-preinstalled-server-arm64+raspi3.img.xz).
    - More details of this image are available on the [Ubuntu Raspberry Pi Wiki](https://wiki.ubuntu.com/ARM/RaspberryPi).
  - You have the image unzipped using a tooll like [Winzip](https://www.winzip.com/landing/download-winzip-v1.html).
  - You have already installed it on the SD Card with something like [etcher](https://etcher.io/).
  - You have access to SSH
    - Default username and password are ubuntu/ubuntu

SSH into the Pi with `ssh ubuntu@ip.address.of.pi` and change the default password. Store this password as you won't have access to it later.

### These instructions should work with Ubuntu 18.0.4 installed on the Pi.

For Ubuntu 18.0.04, install the bionic universe repository
```bash
sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu bionic universe"
```
Then run an apt update to pull in the new repo: `sudo apt-get update`

Some packages are no longer installable from the bionic repository using the 18.0.4 image. You will need to install aptitude to continue.
```
sudo apt-get install aptitude
```
Once Aptitude is installed, any command you issue with aptitude will need confirmation to downgrade the packages. If you see this prompt after issuing `the aptitude install` command:
```
    Accept this solution? [Y/n/q/?]
```
Enter 'n' and press enter. You will then be prompted to downgrade existing packages for the dependencies to be installed. Once the downgrade option is presented:
```
    Accept this solution? [Y/n/q/?]
```
Enter 'y' and press enter. You will then be prompted to proceed with installation.

### Install GNU Radio and other associated packages...

GNU Radio may be installed by manually compiling from source, [the PyBOMBS](https://github.com/gnuradio/pybombs) GNU Radio install management system, or using APT to install from the APT package repository.

Using the Ubuntu package repository and the APT package manager is the currently preferred method for [installing GNU Radio](http://gnuradio.org/redmine/projects/gnuradio/wiki/InstallingGR): 
 
```bash
sudo aptitude install gnuradio gnuradio-dev libuhd-dev libgnuradio-uhd3.7.11
```

For all SDR based devices...
```bash
sudo aptitude install gr-osmosdr libosmosdr0
```

If using HackRF or one of its derivatives...
```bash
sudo aptitude install hackrf libhackrf0 libhackrf-dev
```

### Install tools to compile Trunk Recorder
```bash
sudo apt-get install libusb-1.0-0.dev 
sudo aptitude install git cmake make build-essential libboost-all-dev
```

### Install dependencies for Trunk Recorder
```bash
sudo aptitude install libaacs0 libcppunit-dev libcppunit-1.14-0 libvo-aacenc0 libssl-dev openssl curl libcurl3-gnutls libcurl4 libcurl4-openssl-dev fdkaac sox
```
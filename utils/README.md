
## Build
From the root directory of trunk-recorder, run:
`docker build -t tr-arch -f utils/Dockerfile.arch-latest.dev .`

*On an M1 based Mac:*
`docker build -t tr-arch -f utils/Dockerfile.arch-latest.dev --platform=linux/amd64 .`


## Run
`docker run --platform=linux/amd64 -v ${PWD}:/src -it tr-arch  /bin/bash` 

`docker run -v ${PWD}:/src -it trunkrecorder:dev /bin/bash`  


## Setting a bare metal Raspberry Pi OS install

### Setup Raspbian
This is a [good guide](https://desertbot.io/blog/headless-raspberry-pi-4-ssh-wifi-setup)

#### Download and burn the image
- Download the Raspbery OS Desktop
- Use Etcher to burn it to a MicroSD card
OR
- get the Pi Imager: https://www.raspberrypi.org/software/

#### Setup for headless boot
- *On Mac* `touch /Volumes/boot/ssh`
- Next, add the WiFi info
    - `nano /Volumes/boot/wpa_supplicant.conf`
```
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="NETWORK-NAME"
    psk="NETWORK-PASSWORD"
}
```
- Eject the MicroSD card (first in the OS and then from the reader)
- Put the MicroSD card in the Pi and power it on.
### Remote Access
This is a [good guide](https://www.raspberrypi.org/documentation/computers/remote-access.html)

*These steps should work on a Mac and assume you only have one Pi on the Network*
- Check to see if it is up: `ping raspberrypi.local`
- Clear old known hosts: `ssh-keygen -R raspberrypi.local`
- See if you can connect: `ssh pi@raspberrypi.local`  *default password is: **raspberry** *
- Exit: `exit`
- Create your SSH keys if you don't have them yet: `ssh-keygen`
- Send over you keys: `ssh-copy-id pi@raspberrypi.local`

### Installing the Dependencies
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
- Get the latest Trunk Recorder source:



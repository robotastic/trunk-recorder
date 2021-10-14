# Docker Dev Containers

Docker makes it easier to test on different operating systems

## Build
From the root directory of trunk-recorder, run:
`docker build -t tr-arch -f utils/Dockerfile.arch-latest.dev .`


`docker build -t tr-fedora -f utils/Dockerfile.fedora-35.dev .`


*On an M1 based Mac:*
`docker build -t tr-arch -f utils/Dockerfile.arch-latest.dev --platform=linux/amd64 .`


## Run
This maps in the current directory, so you can try building files from you local machine inside the container. This helps diagnose package errors on other OS.

`docker run -v ${PWD}:/src -it trunkrecorder:dev /bin/bash`  

`docker run -v ${PWD}:/src -it tr-arch /bin/bash`  

`docker run --privileged --ulimit core=-1 -v ${PWD}:/src -v /dev/bus/usb:/dev/bus/usb  -v /var/run/dbus:/var/run/dbus -v /var/run/avahi-daemon/socket:/var/run/avahi-daemon/socket -v /home/luke/trunk-recorder-docker:/app -it tr-arch-aur /bin/bash`

`docker run --privileged --ulimit core=-1 -v ${PWD}:/src -v /dev/bus/usb:/dev/bus/usb  -v /var/run/dbus:/var/run/dbus -v /var/run/avahi-daemon/socket:/var/run/avahi-daemon/socket -v /home/luke/trunk-recorder-docker:/app -it tr-fedora /bin/bash`

*On an M1 based Mac:*
`docker run --platform=linux/amd64 -v ${PWD}:/src -it tr-arch  /bin/bash` 

## Testing Arch AUR

In the */utils* directory, run the following command to build the AUR package:

`docker build -t tr-arch-aur --build-arg AUR_PACKAGE=trunk-recorder -f utils/Dockerfile.arch-latest-aur.dev .`

In a directory with a config.json file you want to test, run this command. It will mirror your current directory to */app*.
`docker run --privileged --ulimit core=-1 -v ${PWD}:/app -v /dev/bus/usb:/dev/bus/usb  -v /var/run/dbus:/var/run/dbus -v /var/run/avahi-daemon/socket:/var/run/avahi-daemon/socket -it tr-arch-aur /bin/bash` 

If you want to capture a core dump: https://stackoverflow.com/questions/28335614/how-to-generate-core-file-in-docker-container

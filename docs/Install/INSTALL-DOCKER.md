---
sidebar_label: 'Docker Install'
sidebar_position: 2
---

# Docker
If you are not going to be modifying the source code, **a [Docker](https://www.docker.com/) based install is the easiest way to get started.** Images are published frequently to  [Docker Hub](https://hub.docker.com/r/robotastic/trunk-recorder). The images have GNURadio 3.8 and all other required dependencies built into it, so it should be ready to go and be a much faster solution than compiling. Images have been built for amd64 (amd64 is used by all modern Intel and AMD CPUs) and most popular flavors of ARM.

To get started, create a directory and place your **config.json** file there and a **talkgroup.csv** file if you are using one. Refer to [Configuring Trunk Recorder](CONFIGURE.md) for instructions on how to create these files.

```bash
docker run -it \
  --privileged -e TZ=$(cat /etc/timezone) --user "$(id -u):$(id -g)" \
  -v $(pwd):/app \
  -v /dev/bus/usb:/dev/bus/usb \
  -v /var/run/dbus:/var/run/dbus \
  -v /var/run/avahi-daemon/socket:/var/run/avahi-daemon/socket \
  robotastic/trunk-recorder:latest
```

To use it as part of a [Docker Compose](https://docs.docker.com/compose/) file:

```yaml
version: '3'
services:
  recorder:
    image: robotastic/trunk-recorder:latest
    container_name: trunk-recorder
    restart: always
    privileged: true
    volumes:
      - /dev/bus/usb:/dev/bus/usb
      - /var/run/dbus:/var/run/dbus 
      - /var/run/avahi-daemon/socket:/var/run/avahi-daemon/socket
      - ./:/app
```

## uploadScript

If you want to run a python script for example with the uploadScript setting, make your shebang #!/usr/bin/python2 or #!/usr/bin/python3 as #!/usr/bin/python is not present.

Also be aware that uploadScript function will pass your (outside of container) script an absolute path of inside the container, so you will need for example a symlink to connect this back to your recordings: ln -s /REPLACE/WITH/PATH/TO/DIR /app

Also note that if you are using uploadScript to connect to for example liquidsoap sockets running outside the container you will need to share/mount them into the container at start by adding -v /var/run/liquidsoap/:/var/run/liquidsoap

## Useful Docker Commands

Once you have gotten your config and testing done you can set docker to run trunk recorder in the background at boot by replacing "-it" with "-d --restart always" and running that once, this will start the image in the background (-d) and start automatically and on failure. 

To view running containers (images started with run) and see their name/id "docker container list", to view image parameters "docker inspect [name/id]", to view logging inside container "docker logs [name/id]" (or logs --follow to tail/watch).

## Events that create new images

Currently, Docker image builds are triggered by the following events:

* After every push to the `master` branch the `edge` tag is built and pushed to Docker Hub.
* Every day at 10 AM UTC the `nightly` tag is built and pushed to Docker Hub.
* When a new [release](https://github.com/robotastic/trunk-recorder/releases) happens the `<version>` and `latest` tags are built and pushed to Docker Hub.


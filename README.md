Trunk Recorder - v4.0 BETA
=======================

## Sponsors
**Do you find Trunk Recorder and OpenMHz useful?** 
Become a [Sponsor](https://github.com/sponsors/robotastic) to help support continued development and operation!
Thank you to everyone who has contributed!

## Overview
Need help? Got something working? Share it!

[![Chat](https://img.shields.io/gitter/room/trunk-recorder/Lobby.svg)](https://gitter.im/trunk-recorder/Lobby?utm_source=share-link&utm_medium=link&utm_campaign=share-link) - and don't forget the [Wiki](https://github.com/robotastic/trunk-recorder/wiki)

Trunk Recorder is able to record the calls on trunked and conventional radio systems. It uses 1 or more Software Defined Radios (SDRs) to do this. The SDRs capture large swathes of RF and then use software to process what was received. [GNURadio](https://gnuradio.org/) is used to do this processing because it provides lots of convenient RF blocks that can be pieced together to allow for complex RF processing. The libraries from the amazing [OP25](http://op25.osmocom.org/trac/wiki) project are used for a lot of the P25 functionality. Multiple radio systems can be recorded at the same time.


Trunk Recorder currently supports the following:

 - Trunked P25 & SmartNet Systems
 - Conventional P25 & analog systems, where each group has a dedicated RF channel
 - P25 Phase 1, P25 Phase 2 & Analog voice channels

Supported platforms:

**Ubuntu** (18.04,  20.04, 21.04); **Raspberry Pi** (Raspberry OS/Raspbian & Ubuntu 21.04); **Arch Linux** (2021.09.20); **Debian** (9.x); **macOS**

...and SDRs:

RTL-SDR dongles; HackRF; Ettus USRP B200, B210, B205; BladeRF; Airspy

---

## Version Notes
### V4.0
- The executable generated has changed from `recorder` to `trunk-recorder` to help differentiate it from other applications that maybe installed.
- A new method is used to detect the end of call. Instead of waiting for a timeout after the last trunking message is recieved, the voice channel is monitored for messages announcing the end of a transmission. Each transmission is stored in a separate file and then merged together after a talkgroup stops using a frequency.
- The format for audio filenames has changed slightly. 
  It is now: [ Talkgroup ID ]\_[ Unix Timestamp ]-[ Frequency ]-call\_[ Call Counter ].wav

*[See past notes in the ChangeLog. If you upgrade and things are not working, check here](CHANGELOG.md)

---

## Install

|              |           Docker            |                   Ubuntu                   |       RaspberryOS       |              Arch Linux               |                Homebrew                 |                MacPorts                 |
| ------------ | :-------------------------: | :----------------------------------------: | :---------------------: | :-----------------------------------: | :-------------------------------------: | :-------------------------------------: |
| Linux        | [📄](docs/INSTALL-DOCKER.md) | [📄](docs/INSTALL-LINUX.md#**ubuntu-2104**) |                         | [📄](docs/INSTALL-LINUX.md#arch-linux) |                                         |                                         |
| Raspberry Pi | [📄](docs/INSTALL-DOCKER.md) |          [📄](docs/INSTALL-PI.md)           | [📄](docs/INSTALL-PI.md) |                                       |                                         |                                         |
| MacOS        |                             |                                            |                         |                                       | [📄](docs/INSTALL-MAC.md#using-homebrew) | [📄](docs/INSTALL-MAC.md#using-macports) |



### Setup
* [Configuring a system](docs/CONFIGURE.md)

### Playback & Sharing
By default, Trunk Recorder just dumps a lot of recorded files into a directory. Here are a couple of options to make it easier to browse through recordings and share them on the Internet.
* [OpenMHz](https://github.com/robotastic/trunk-recorder/wiki/Uploading-to-OpenMHz) - This is my free hosted platform for sharing recordings
* [Trunk Player](https://github.com/ScanOC/trunk-player) - A great Python based server, if you want to you want to run your own
* [Rdio Scanner](https://github.com/chuot/rdio-scanner) - Provide a good looking, scanner style interface for listening to Trunk Recorder
* [Broadcastify Calls (API)](https://forums.radioreference.com/threads/405236/)-[Wiki](https://wiki.radioreference.com/index.php/Broadcastify-Calls-Trunk-Recorder)
* [Broadcastify via Liquidsoap](https://github.com/robotastic/trunk-recorder/wiki/Streaming-online-to-Broadcastify-with-Liquid-Soap)
* [audioplayer.php](https://github.com/robotastic/trunk-recorder/wiki/Using-audioplayer.php)

### Troubleshooting

If are having trouble, check out the [FAQ](docs/FAQ.md) and/or ask a question on [![Chat](https://img.shields.io/gitter/room/trunk-recorder/Lobby.svg)](https://gitter.im/trunk-recorder/Lobby?utm_source=share-link&utm_medium=link&utm_campaign=share-link)

___

### How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording.

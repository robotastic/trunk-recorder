Trunk Recorder - v4.5.0
=======================

## Sponsors
**Do you find Trunk Recorder and OpenMHz useful?** 
Become a [Sponsor](https://github.com/sponsors/robotastic) to help support continued development and operation!
Thank you to everyone who has contributed!

## Overview
Need help? Got something working? Share it!

[Discord Server](https://discord.gg/btJAhESnks) - and don't forget the [Wiki](https://github.com/robotastic/trunk-recorder/wiki)

Trunk Recorder is able to record the calls on trunked and conventional radio systems. It uses 1 or more Software Defined Radios (SDRs) to do this. The SDRs capture large swathes of RF and then use software to process what was received. [GNU Radio](https://gnuradio.org/) is used to do this processing because it provides lots of convenient RF blocks that can be pieced together to allow for complex RF processing. The libraries from the amazing [OP25](http://op25.osmocom.org/trac/wiki) project are used for a lot of the P25 functionality. Multiple radio systems can be recorded at the same time.


Trunk Recorder currently supports the following:

 - Trunked P25 & SmartNet Systems
 - Conventional P25 & analog systems, where each group has a dedicated RF channel
 - P25 Phase 1, P25 Phase 2 & Analog voice channels

Supported platforms:

**Ubuntu** (18.04,  20.04, 21.04, 22.04); **Raspberry Pi** (Raspberry OS/Raspbian & Ubuntu 21.04, 22.04); **Arch Linux** (2021.09.20); **Debian** (9.x); **macOS**

GNU Radio 3.7 - 3.10

...and SDRs:

RTL-SDR dongles; HackRF; Ettus USRP B200, B210, B205; BladeRF; Airspy

---

## Version Notes
### V4.0
- The executable generated has changed from `recorder` to `trunk-recorder` to help differentiate it from other applications that maybe installed.
- A new method is used to detect the end of call. Instead of waiting for a timeout after the last trunking message is received, the voice channel is monitored for messages announcing the end of a transmission. Each transmission is stored in a separate file and then merged together after a talkgroup stops using a frequency.
- The format for audio filenames has changed slightly. 
  It is now: [ Talkgroup ID ]\_[ Unix Timestamp ]-[ Frequency ]-call\_[ Call Counter ].wav

*[See past notes in the ChangeLog. If you upgrade and things are not working, check here](CHANGELOG.md)

---

## Install

|              |           Docker            |                   Ubuntu                   |       RaspberryOS       |              Arch Linux               |                Homebrew                 |                MacPorts                 |
| ------------ | :-------------------------: | :----------------------------------------: | :---------------------: | :-----------------------------------: | :-------------------------------------: | :-------------------------------------: |
| Linux        | [📄](docs/INSTALL-DOCKER.md) | [📄](docs/INSTALL-LINUX.md#**ubuntu-2104**) |                         | [📄](docs/INSTALL-LINUX.md#arch-linux) |                                         |                                         |
| Raspberry Pi | [📄](docs/INSTALL-DOCKER.md) |          [📄](docs/INSTALL-PI.md)           | [📄](docs/INSTALL-PI.md) [🎬](https://youtu.be/DizBtDZ6kE8) |                                       |                                         |                                         |
| MacOS        |                             |                                            |                         |                                       | [📄](docs/INSTALL-MAC.md#using-homebrew) | [📄](docs/INSTALL-MAC.md#using-macports) |



### Setup
* [Configuring a system](docs/CONFIGURE.md)

### Playback & Sharing
By default, Trunk Recorder just dumps a lot of recorded files into a directory. Here are a couple of options to make it easier to browse through recordings and share them on the Internet.
* [OpenMHz](https://github.com/robotastic/trunk-recorder/wiki/Uploading-to-OpenMHz): This is my free hosted platform for sharing recordings
* [Trunk Player](https://github.com/ScanOC/trunk-player): A great Python based server, if you want to you want to run your own
* [Rdio Scanner](https://github.com/chuot/rdio-scanner): Provide a good looking, scanner style interface for listening to Trunk Recorder
* Broadcastify Calls (API): see Radio Reference [forum thread](https://forums.radioreference.com/threads/405236/) and [wiki page](https://wiki.radioreference.com/index.php/Broadcastify-Calls-Trunk-Recorder)
* [Broadcastify via Liquidsoap](https://github.com/robotastic/trunk-recorder/wiki/Streaming-online-to-Broadcastify-with-Liquid-Soap)
* [audioplayer.php](https://github.com/robotastic/trunk-recorder/wiki/Using-audioplayer.php)
* [rosecitytransit's Live Web page](https://github/rosecitytransit/trunk-recorder-daily-log)

### Plugins

* [MQTT Status](https://github.com/robotastic/trunk-recorder-mqtt-status): Publishes the current status of a Trunk Recorder instance over MQTT
* [MQTT Statistics](https://github.com/robotastic/trunk-recorder-mqtt-statistics): Publishes statistics about a Trunk Recorder instance over MQTT
* [Decode rates logger](https://github.com/rosecitytransit/trunk-recorder-decode-rate): Logs trunking control channel decode rates to a CSV file, and includes a PHP file that outputs an SVG graph
* [Daily call log and live Web page]((https://github.com/rosecitytransit/trunk-recorder-daily-log): Creates a daily log of calls (instead of just individual JSON files) and includes an updating PHP Web page w/audio player

### Troubleshooting

If are having trouble, check out the [FAQ](docs/FAQ.md) and/or ask a question on the [Discord Server](https://discord.gg/trunk-recorder) 

___

### How Trunking Works
For those not familiar, trunking systems allow a large number of user groups to share a limited number of radio frequencies by temporarily, dynamically assigning radio frequencies to talkgroups (channels) on-demand. It is understood that most user groups actually use the radio very sporadically and don't need a dedicated frequency. 

Most trunking system types (such as SmartNet and P25) set aside one of the radio frequencies as a "control channel" that manages and broadcasts radio frequency assignments. When someone presses the Push to Talk button on their radio, the radio sends a message to the system which then assigns a voice frequency and broadcasts a Channel Grant message about it on the control channel. This lets the radio know what frequency to transmit on and tells other radios set to the same talkgroup to listen.

In order to follow all of the transmissions, Trunk Recorder constantly listens to and decodes the control channel. When a frequency is granted to a talkgroup, Trunk Recorder creates a monitoring process which decodes the portion of the radio spectrum for that frequency from the SDR that is already pulling it in.

No message is transmitted on the control channel when a conversation on a talkgroup is over. The monitoring process keeps track of transmissions and if there has been no activity for a specified period, it ends the recording.

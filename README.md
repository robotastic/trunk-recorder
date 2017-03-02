Trunk Recorder - v2.1.2
=================
### Note: The format for the Config.json file changed in v2.x.

Need help? Got something working? Share it!


[![Chat](http://img.shields.io/badge/gitter-USER / REPO-blue.svg)]( https://gitter.im/trunk-recorder/Lobby?utm_source=share-link&utm_medium=link&utm_campaign=share-link) - [Google Groups](https://groups.google.com/d/forum/trunk-recorder) - and don't forget the [Wiki](https://github.com/robotastic/trunk-recorder/wiki)

Trunk Recorder is able to record the calls on trunked and conventional radio systems. It uses 1 or more Software Defined Radios (SDRs) to do this. The SDRs capture large swatches of RF and then use software to process what was received. GNURadio is used to do this processing because it provides lots of convenient RF blocks that can be pieced together to allow for complex RF processing. Multiple radio systems can be recorded at the same time.

Trunk Recorder currently supports the following:
 - Trunked P25 & SmartNet Systems
 - Conventional P25 & analog systems, where each group has a dedicated RF channel
 - SDRs that use the OsmoSDR source ( HackRF, RTL - TV Dongles, BladeRF, and more)
 - Ettus USRPs
 - P25 Phase 1 & Analog voice channels

I have tested things on both Unbuntu: 16.04, 14.04; OSX 10.10, OSX 10.11, 10.12. I have been using it with an Ettus b200, 3xRTL-SDR dongles and a HackRF Jawbreaker.

## Wiki Pages
### Installing
* [Docker](https://github.com/robotastic/trunk-recorder/wiki/Docker-Install)
* [Ubuntu 16.04](https://github.com/robotastic/trunk-recorder/wiki/Ubuntu-16.04-Install)
* [Raspberry Pi - Jessie](https://github.com/robotastic/trunk-recorder/wiki/Raspberry-Pi-Jessie-Install)

### Setup
* [Configuring a system](https://github.com/robotastic/trunk-recorder/wiki/Configuring-a-System)

###Playback & Sharing
By default, Trunk Recorder just dumps a lot of recorded files into a directory. Here are a couple of options to make it easier to browse through recordings and share them on the Internet.
* [OpenMHz](https://github.com/robotastic/trunk-recorder/wiki/Uploading-to-OpenMHz) - This is my free hosted platform for sharing recordings
* [Trunk Player](https://github.com/ScanOC/trunk-player) - A great Python based server, if you want to you want to run your own

* [FAQ](https://github.com/robotastic/trunk-recorder/wiki/FAQ)

___

##Install

###Requirements
 - GNURadio 3.7

**OSX**

If you are on OSX, the [MacPorts](https://gnuradio.org/redmine/projects/gnuradio/wiki/MacInstall) install of GNU Radio has worked for me.

###Setting up [GNU Radio](http://gnuradio.org/) on a fresh [Ubuntu](http://www.ubuntu.com/) Version 16.04 [Release](http://releases.ubuntu.com/16.04/)

There are a few methods to install GNU Radio. Source, [PyBOMBS](https://github.com/gnuradio/pybombs), or Distribution package manager. In this setup we will be using apt-get to install GNURadio (fastest method I have used). GNU Radio Version in apt-get as of 07/11/2016 is 3.7.9.  
Using a package manager is the currently preferred method from the GNU Radio Project, [Installing GNU Radio](http://gnuradio.org/redmine/projects/gnuradio/wiki/InstallingGR).

**Using apt-get to get GNU Radio and other prerequisites for Trunk Recorder**

Update currently install packages

`sudo apt-get update`  
`sudo apt-get upgrade`

Install GNU Radio with other prerequisites  
`sudo apt-get install gnuradio gr-osmosdr libhackrf-dev libuhd-dev`  

Install tools to compile Trunk Recorder  
`sudo apt-get install git cmake build-essential libboost-all-dev libusb-1.0-0.dev libssl-dev`  

Get source for Trunk Recorder  
Note: I put all my Radio related code into ~/radio/, change this as you wish  

`mkdir ~/radio`  
`cd ~/radio/`  
`git clone https://github.com/robotastic/trunk-recorder.git`  
`cd trunk-recorder`  
`cmake .`  
`make`  

**Running trunk recorder.**

If all goes well you should now have the executable named recorder.  
Before you can run anything, you need to create a `config.json` file ( see below ).
After you have done that, just run:  
`./recorder`

##Configure
Configuring Trunk Recorder and getting things setup can be rather complex. I am looking to make things simpler in the future.

**config.json**

This file is used to configure how Trunk Recorder is setup. It defines the SDRs that are available and the trunk system that will be recorded. The following is an example for my local system in DC, using an Ettus B200:

```
{
    "sources": [{
        "center": 857000000.0,
        "rate": 8000000.0,
        "squelch": -50,
        "error": 0,
        "gain": 40,
        "antenna": "TX/RX",
        "digitalRecorders": 2,
        "driver": "usrp",
        "device": "",
        "modulation": "qpsk"
    }],
    "systems": [{
        "control_channels": [855462500],
        "type": "p25",
        "talkgroupsFile": "ChanList.csv"
    }]
}
```
Here are the different arguments:
 - **sources** - an array of JSON objects that define the different SDRs available. The following options are used to configure each Source:
   - **center** - the center frequency in Hz to tune the SDR to
   - **rate** - the sampling rate to set the SDR to, in samples / second
   - **squelch** - Analog Squelch, my rtl-sdr's are around -60. [0 = Disabled] _Squelch needs to be set if the System using the source is conventional._
   - **error** - the tuning error for the SDR in Hz. This is the difference between the target value and the actual value. So if you wanted to recv 856MHz but you had to tune your SDR to 855MHz to actually receive it, you would set this to -1000000. You should also probably get a new SDR if it is off by this much.
   - **gain** - the RF gain to set the SDR to. Use a program like GQRX to find a good value.
   - **ifGain** - [hackrf only] sets the if gain.
   - **bbGain** - [hackrf only] sets the bb gain.
   - **mixGain** - [AirSpy only] sets the mix gain.
   - **lnaGain** - [AirSpy only] sets the lna gain.
   - **antenna** - [usrp] lets you select which antenna jack to user on devices that support it
   - **digitalRecorders** - the number of Digital Recorders to have attached to this source. This is essentially the number of simultaneous calls you can record at the same time in the frequency range that this Source will be tuned to. It is limited by the CPU power of the machine. Some experimentation might be needed to find the appropriate number.
   - **digitLevels** - the amount of amplification that will be applied to the digital audio. The value should be between 1-16. The default value is 8.
   - **modulation** - the type of modulation that the system uses. The options are *qpsk* & *fsk4*. It is possible to have a mix of sources using fsk4 and qpsk demodulation.
   - **analogRecorders** - the number of Analog Recorder to have attached to this source. This is the same as Digital Recorders except for Analog Voice channels.
   - **analogLevels** - the amount of amplification that will be applied to the analog audio. The value should be between 1-32. The default value is 8.
   - **debugRecorders** - the number of Debug Recorder to have attached to this source. Debug Recorders capture a raw sample that you can examine later using GNURadio Companion. This is helpful if you want to fine tune your the error and gain for this Source.
   - **driver** - the GNURadio block you wish to use for the SDR. The options are *usrp* & *osmosdr*.
   - **device** - osmosdr device name and possibly serial number or index of the device, see [osmosdr page](http://sdr.osmocom.org/trac/wiki/GrOsmoSDR) for each device and parameters. You only need to do this if there are more than one. (bladerf=00001 for BladeRF with serial 00001 or rtl=00923838 for RTL-SDR with serial 00923838, just airspy for an airspy) It seems that when you have 5 or more RTLSDRs on one system you need to decrease the buffer size. I think it has something to do with the driver. Try adding buflen: "device": "rtl=serial_num,buflen=65536", there should be no space between buflen and the comma
 - **systems** - An array of JSON objects that define the trunking systems that will be recorded. The following options are used to configure each System.
   - **control_channels** - *(For trunked systems)* an array of the control channel frequencies for the system, in Hz. The frequencies will automatically be cycled through if the system moves to an alternate channel.
   - **channels** - *(For conventional systems)* an array of the channel frequencies, in Hz, used for the system. The channels get assigned a virtual talkgroup number based upon their position in the array. Squelch levels need to be specified for the Source(s) being used.
   - **type** - the type of trunking system. The options are *smartnet*, *p25*,  *conventional* & *conventionalP25*.
   - **talkgroupsFile** - this is a CSV file that provides information about the talkgroups. It determines whether a talkgroup is analog or digital, and what priority it should have. This file should be located in the same directory as the trunk-recorder executable.
   - **shortName** - this is a nickname for the system. It is used to help name and organize the recordings from this system. It should be 4-6 letters with no spaces.
   - **uploadScript** - this script is called after each recording has finished. Checkout *encode-upload.sh.sample* as an example. The script should be located in the same directory as the trunk-recorder executable.
 - **defaultMode** - Default mode to use when a talkgroups is not listed in the **talkgroupsFile** the options are *digital* or *analog*.
 - **captureDir** - the complete path to the directory where recordings should be saved.
 - **callTimeout** - a Call will stop recording and save if it has not received anything on the control channel, after this many seconds. The default is 3.

**ChanList.csv**

This file provides info on the different talkgroups in a trunking system. A lot of this info can be found on the Radio Reference website. You need to be a site member to download the table for your system. If you are not, try clicking on the "List All in one table" link, selecting everything in the table and copying it into Excel or a spreadsheet.

You will have to add an additional column that adds a priority for each talkgroup. You need that number of recorders available to record a call at that priority. So, 1 is the highest, you would need 2 recorders available to record a priority 2, 3 record for a priority 3 and so on.

The Trunk Record program really only uses the priority information and the Dec Talkgroup ID. The Website uses the same file though to help display information about each talkgroup.

Here are the column headers and some sample data:

| DEC |	HEX |	Mode |	Alpha Tag	| Description	| Tag |	Group | Priority |
|-----|-----|------|-----------|-------------|-----|-------|----------|
|101	| 065	| D	| DCFD 01 Disp	| 01 Dispatch |	Fire Dispatch |	Fire | 1 |
|2227 |	8b3	| D	| DC StcarYard	| Streetcar Yard |	Transportation |	Services | 3 |

###Multiple SDR
Most trunk systems use a wide range of spectrum. Often a more powerful SDR is needed to have enough bandwidth to capture all of the potential channels that a system may broadcast on. However it is possible to use multiple SDRs working together to cover all of the channels. This means that you can use a bunch of cheap RTL-SDR to capture an entire system.

In addition to being able to use a cheaper SDR, it also helps with performance. When a single SDR is used, each of the Recorders gets fed all of the sampled signal. Each Recorder needs to cut down the multi-megasamples per second into a small 12.5Khz sliver. When you use multiple SDRs, each SDR is capturing only partial slice of the system so the Recorders have to cut down a much smaller amount of sample to get to the sliver they are interested in. This menans that you can have a lot more recorders running!

To use mutliple SDRs, simply define additional Sources in the Source array. The `confing-multi-rtl.json.sample` has an example of how to do this. In order to tell the different SDRs apart and make sure they get the right error correction value, give them a serial number using the `rtl_eeprom -s` command and then specifying that number in the `device` setting for that Source, `rtl=2`.

###How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording.

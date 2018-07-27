Trunk Recorder - v3.0.1
=======================

Need help? Got something working? Share it!


[![Chat](https://img.shields.io/gitter/room/trunk-recorder/Lobby.svg)](https://gitter.im/trunk-recorder/Lobby?utm_source=share-link&utm_medium=link&utm_campaign=share-link) - [Google Groups](https://groups.google.com/d/forum/trunk-recorder) - and don't forget the [Wiki](https://github.com/robotastic/trunk-recorder/wiki)

Trunk Recorder is able to record the calls on trunked and conventional radio systems. It uses 1 or more Software Defined Radios (SDRs) to do this. The SDRs capture large swatches of RF and then use software to process what was received. [GNURadio](https://gnuradio.org/) is used to do this processing because it provides lots of convenient RF blocks that can be pieced together to allow for complex RF processing. The libraries from the amazing [OP25](http://op25.osmocom.org/trac/wiki) project are used for a lot of the P25 functionality. Multiple radio systems can be recorded at the same time.

Trunk Recorder currently supports the following:
 - Trunked P25 & SmartNet Systems
 - Conventional P25 & analog systems, where each group has a dedicated RF channel
 - SDRs that use the OsmoSDR source (HackRF, RTL - TV Dongles, BladeRF, and more)
 - Ettus USRPs
 - P25 Phase 1, **P25 Phase 2** & Analog voice channels

Trunk Recorder has been tested on Ubuntu (14.04, 16.04, 16.10, 17.04, 17.10 & 18.04), Arch Linux (2017.03.01), Debian 9.x and macOS (10.10, 10.11, 10.12, 10.13, 10.14). It has been successfully used with several SDRs including the Ettus USRP B200, B210, B205, a bank of 3 RTL-SDR dongles, and the HackRF Jawbreaker.

# Wiki Pages

## Install

### Install Required Prequisites
* [Docker](https://github.com/robotastic/trunk-recorder/wiki/Docker-Install)
* [Ubuntu](https://github.com/robotastic/trunk-recorder/wiki/Ubuntu)
* [Arch Linux](https://github.com/robotastic/trunk-recorder/wiki/Arch-Linux)
* [macOS](https://github.com/robotastic/trunk-recorder/wiki/macOS)
* [Raspberry Pi Stretch Install](https://github.com/robotastic/trunk-recorder/wiki/Raspberry-Pi-Stretch-Install) (Also works for the ASUS Tinker Board [S])

### Building
* [Building Trunk Recorder](https://github.com/robotastic/trunk-recorder/wiki/Building-Trunk-Recorder)

### Setup
* [Configuring a system](https://github.com/robotastic/trunk-recorder/wiki/Configuring-a-System)

### Running
* [Running Trunk Recorder](https://github.com/robotastic/trunk-recorder/wiki/Running-Trunk-Recorder)

### Playback & Sharing
By default, Trunk Recorder just dumps a lot of recorded files into a directory. Here are a couple of options to make it easier to browse through recordings and share them on the Internet.
* [OpenMHz](https://github.com/robotastic/trunk-recorder/wiki/Uploading-to-OpenMHz) - This is my free hosted platform for sharing recordings
* [Trunk Player](https://github.com/ScanOC/trunk-player) - A great Python based server, if you want to you want to run your own

* [FAQ](https://github.com/robotastic/trunk-recorder/wiki/FAQ)

___

## Configure
Configuring Trunk Recorder and getting things setup can be rather complex. I am looking to make things simpler in the future.

**config.json**

This file is used to configure how Trunk Recorder is setup. It defines the SDRs that are available and the trunk system that will be recorded. Trunk Recorder will look for a *config.json* file in the same directory as it is being run in. You can point it to a different config file by using the *--config* argument on the command line, for example: `./recorder --config=examples/config-wmata-rtl.json`. The following is an example for my local system in DC, using an Ettus B200:

```json
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
   - **lnaGain** - [AirSpy/bladeRF only] sets the lna gain.
   - **vga1Gain** - [bladeRF only] sets the vga1 gain.
   - **vga2Gain** - [bladeRF only] sets the vga2 gain.
   - **antenna** - [usrp] lets you select which antenna jack to user on devices that support it
   - **digitalRecorders** - the number of Digital Recorders to have attached to this source. This is essentially the number of simultaneous calls you can record at the same time in the frequency range that this Source will be tuned to. It is limited by the CPU power of the machine. Some experimentation might be needed to find the appropriate number.
   - **digitalLevels** - the amount of amplification that will be applied to the digital audio. The value should be between 1-16. The default value is 8.
   - **modulation** - the type of modulation that the system uses. The options are *qpsk* & *fsk4*. It is possible to have a mix of sources using fsk4 and qpsk demodulation.
   - **analogRecorders** - the number of Analog Recorder to have attached to this source. This is the same as Digital Recorders except for Analog Voice channels.
   - **analogLevels** - the amount of amplification that will be applied to the analog audio. The value should be between 1-32. The default value is 8.
   - **debugRecorders** - the number of Debug Recorder to have attached to this source. Debug Recorders capture a raw sample that you can examine later using GNURadio Companion. This is helpful if you want to fine tune your the error and gain for this Source.
   - **driver** - the GNURadio block you wish to use for the SDR. The options are *usrp* & *osmosdr*.
   - **device** - osmosdr device name and possibly serial number or index of the device, see [osmosdr page](http://sdr.osmocom.org/trac/wiki/GrOsmoSDR) for each device and parameters. You only need to do this if there are more than one. (`bladerf=00001` for BladeRF with serial 00001 or `rtl=00923838` for RTL-SDR with serial 00923838, just airspy for an airspy) It seems that when you have 5 or more RTLSDRs on one system you need to decrease the buffer size. I think it has something to do with the driver. Try adding buflen: `"device": "rtl=serial_num,buflen=65536"`, there should be no space between buflen and the comma
 - **systems** - An array of JSON objects that define the trunking systems that will be recorded. The following options are used to configure each System.
   - **control_channels** - *(For trunked systems)* an array of the control channel frequencies for the system, in Hz. The frequencies will automatically be cycled through if the system moves to an alternate channel.
   - **channels** - *(For conventional systems)* an array of the channel frequencies, in Hz, used for the system. The channels get assigned a virtual talkgroup number based upon their position in the array. Squelch levels need to be specified for the Source(s) being used.
   - **alphatags** - *(Optional, For conventional systems)* an array of the alpha tags, these will be outputed to the logfiles *talkgroupDisplayFormat* is set to include tags. Alpha tags will be applied to the *channels* in the order the values appear in the array. 
   - **type** - the type of trunking system. The options are *smartnet*, *p25*,  *conventional* & *conventionalP25*.
   - **talkgroupsFile** - this is a CSV file that provides information about the talkgroups. It determines whether a talkgroup is analog or digital, and what priority it should have. This file should be located in the same directory as the trunk-recorder executable.
   - **recordUnknown** - record talkgroups if they are not listed in the Talkgroups File. The options are *true* and *false* (without quotes). The default is *true*.
   - **shortName** - this is a nickname for the system. It is used to help name and organize the recordings from this system. It should be 4-6 letters with no spaces.
   - **uploadScript** - this script is called after each recording has finished. Checkout *encode-upload.sh.sample* as an example. The script should be located in the same directory as the trunk-recorder executable.
   - **audioArchive** - should the recorded audio files be kept after successfully uploading them. The options are *true* and *false* (without quotes). The default is *true*.
   - **callLog** - should a json file with the call details be generated. The options are *true* and *false* (without quotes). The default is *true*.
   - **bandplan** - [SmartNet only] this is the SmartNet bandplan that will be used. The options are *800_standard*, *800_reband*, *800_splinter*, and *400_custom*. *800_standard* is the default.
   - **bandplanBase** - [SmartNet, 400_custom only] this is for the *400_custom* bandplan only. This is the base frequency, specified in Hz.
   - **bandplanHigh** - [SmartNet, 400_custom only] this is the highest channel in the system, specified in Hz.
   - **bandplanSpacing** - [SmartNet, 400_custom only] this is the channel spacing, specified in Hz. Typically this is *25000*.
   - **bandplanOffset** - [SmartNet, 400_custom only] this is the offset used to calculate frequencies.
   - **talkgroupDisplayFormat** - the display format for talkgroups in the console and log file. the options are *id*, *id_tag*, *tag_id*. The default is *id*. [*id_tag* and *tag_id* is only valid if **talkgroupsFile** is specified]
   - **delayCreateOutput** - [conventionalP25 only] this will delay the creation of the output file until there is activity, The options are *true* or *false*, without quotes. The default is *false*. 
   - **hideEncrypted** - hide encrypted talkgroups log entries, The options are *true* or *false*, without quotes. The default is *false*. 
   - **hideUnknownTalkgroups** - hide unknown talkgroups from log, The options are *true* or *false*, without quotes. The default is *false*. 
 - **defaultMode** - Default mode to use when a talkgroups is not listed in the **talkgroupsFile** the options are *digital* or *analog*.
 - **captureDir** - the complete path to the directory where recordings should be saved.
 - **callTimeout** - a Call will stop recording and save if it has not received anything on the control channel, after this many seconds. The default is 3.
 - **logFile** - save the console output to a file. The options are *true* or *false*, without quotes. The default is *false*.
 - **frequencyFormat** - the display format for frequencies to display in the console and log file. The options are *exp*, *mhz* & *hz*. The default is *exp*.
 - **statusServer** - The URL for a WebSocket connect. Trunk Recorder will send JSON formatted update message to this address. HTTPS is currently not supported, but will be in the future. OpenMHz does not support this currently. [JSON format of messages](STATUS-JSON.md)
 - **controlWarnRate** - Log the control channel decode rate when it falls bellow this threshold. The default is *10*. The value of *-1* will always log the decode rate.
 - **statusAsString** - Show status as strings instead of numeric values The options are *true* or *false*, without quotes. The default is *false*.

**talkgroupsFile**

This file provides info on the different talkgroups in a trunking system. A lot of this info can be found on the [Radio Reference](http://www.radioreference.com/) website. You need to be a Radio Reference member to download the table for your system preformatted as a CSV file. If you are not a Radio Reference member, try clicking on the "List All in one table" link, selecting everything in the table and copying it into Excel or a spreadsheet, and then exporting or saving as a CSV file.

**Note** - Fields in preformatted CSV downloads from Radio Reference are now in a different order than Trunk Recorder expects. See below for the correct field order. Additionally, Radio Reference inserts a header line at the tope of the CSV file which should be removed.

You may add an additional column that adds a priority for each talkgroup. The priority field specifies the number of recorders the system must have available to record a new call for the talkgroup. For example, a priority of 1, the highest means as long as at least a single recorder is available, the system will record the new call. If the priority is 2, the system would at least 2 free recorders to record the new call, and so on. If there is no priority set for a talkgroup entry, a prioity of 1 is assumed.

The Trunk Record program really only uses the priority information and the Dec Talkgroup ID. The Website uses the same file though to help display information about each talkgroup.

Here are the column headers and some sample data:

| DEC |	HEX |	Mode |	Alpha Tag	| Description	| Tag |	Group | Priority |
|-----|-----|------|-----------|-------------|-----|-------|----------|
|101	| 065	| D	| DCFD 01 Disp	| 01 Dispatch |	Fire Dispatch |	Fire | 1 |
|2227 |	8b3	| D	| DC StcarYard	| Streetcar Yard |	Transportation |	Services | 3 |

### Multiple SDR
Most trunk systems use a wide range of spectrum. Often a more powerful SDR is needed to have enough bandwidth to capture all of the potential channels that a system may broadcast on. However it is possible to use multiple SDRs working together to cover all of the channels. This means that you can use a bunch of cheap RTL-SDR to capture an entire system.

In addition to being able to use a cheaper SDR, it also helps with performance. When a single SDR is used, each of the Recorders gets fed all of the sampled signal. Each Recorder needs to cut down the multi-megasamples per second into a small 12.5Khz sliver. When you use multiple SDRs, each SDR is capturing only partial slice of the system so the Recorders have to cut down a much smaller amount of sample to get to the sliver they are interested in. This menans that you can have a lot more recorders running!

To use mutliple SDRs, simply define additional Sources in the Source array. The `confing-multi-rtl.json.sample` has an example of how to do this. In order to tell the different SDRs apart and make sure they get the right error correction value, give them a serial number using the `rtl_eeprom -s` command and then specifying that number in the `device` setting for that Source, `rtl=2`.

### How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording.

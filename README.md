Trunk Recorder
=================

Trunk Recorder is able to record the calls on a trunked radio system. It uses 1 or more Software Defined Radios (SDRs) to do. The SDRs capture large swatches of RF and then use software to process what was recieved. GNURadio is used to do this processing and provides lots of convienent RF blocks that can be pieced together to do complex RF processing. Right now it can only record one Trunked System at a time.

Trunk Recorder currently supports the following:
 - P25 & SmartNet Trunking Systems
 - SDRs that use the OsmoSDR source ( HackRF, RTL - TV Dongles, BladeRF, and more)
 - Ettus USRP
 - P25 Phase 1 & Analog voice
 
I have tested things on both Unbuntu 14.04, OSX 10.10, & OSX 10.11. I have been using it with an Ettus b200, 3xRTL-SDR dongles and a HackRF Jawbreaker.

##Compile

###Requirements
 - GNURadio 3.7
 - (GR-DSD & OP25 used to be required, but I just bundled in a fork of OP25)
  
**GNURadio**

It is important to have a very recent version of GnuRadio (GR). There was a bug in earlier versions that messed up the smartnet trunking. Make sure your install is up to date if you are having trouble deoding smartnet trunking.

If you are running Linux, the easiest way to install GR is by using [Pybombs](http://gnuradio.org/redmine/projects/pybombs/wiki). After you have installed using pybombs, make sure you setup you Environment variables. In your pybombs directory, run: `./pybombs env` and then load them `source $prefix/setup_env.sh`, with $prefix being the directory you installed GR in.

If you are on OSX, the [MacPorts](https://gnuradio.org/redmine/projects/gnuradio/wiki/MacInstall) install has worked for me.

**Boost**
The Boost Libraries are needed. On Linux box, it can't hurt to do:
```
sudo apt-get install libboost-all-dev
```

**GR-DSD**

*GR-DSD is no longer needed. I couldn't get it to do a good job of decoding QPSK systems.*

**OP25**

*OP25 can do a good job of decoding QPSK systems. It has a bug though that causes it use lots of CPU. I have a branch that fixes this and have bundled it direclty with Trunk Recorder. You do not need a seperate install of OP25.*

###Trunk Recorder
Okay, with that out of the way, here is how you compile Trunk Recorder:
```
git clone https://github.com/robotastic/trunk-recorder.git
cd trunk-recorder
cmake -DCMAKE_PREFIX_PATH=/path/to/GR/install   .
make 
```
Hopefully this should compile with no errors.

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
        "device": ""
    }],
    "system": {
        "control_channels": [855462500],
        "type": "p25",
        "modulation": "QPSK"
    },
    "talkgroupsFile": "ChanList.csv"
}
```
Here are the different arguments:
 - **sources** - an array of JSON objects that define the different SDRs available and how to configure them
   - **center** - the center frequency in Hz to tune the SDR to
   - **rate** - the sampling rate to set the SDR to, in samples / second
   - **squelch** - Analog Squelch, Set each source when using multiple sources, my rtl-sdr's are around -60. [0 = Disabled]
   - **error** - the tuning error for the SDR in Hz. This is the difference between the target value and the actual value. So if you wanted to recv 856MHz but you had to tune your SDR to 855MHz to actually recieve it, you would set this to -1000000. You should also probably get a new SDR.
   - **gain** - the RF gain to set the SDR to. Use a program like GQRX to find a good value.
   - **ifGain** - [hackrf only] sets the ifgain.
   - **bbGain** - [hackrf only] sets the bbgain.
   - **antenna** - [usrp] lets you select which antenna jack to user on devices that support it
   - **digitalRecorders** - the number of Digital Recorders to have attached to this source. This is essentaully the number of simultanious call you can record at the same time in the frequency range that this SDR will be tuned to. It is limited by the CPU power of the machine. Some experimentation might be needed to find the appropriate number. It will use DSD or OP25 to decode the P25 CAI voice.
   - **analogRecorders** - the number of Analog Recorder to have attached to this source. This is the same as Digital Recorders except for Analog Voice channels.
   - **driver** - the GNURadio block you wish to use for the SDR. The options are *usrp* & *osmosdr*.
   - **device** - osmosdr device name and posiably serial number or index of the device, see [osmosdr page](http://sdr.osmocom.org/trac/wiki/GrOsmoSDR) for each device and parameters. You only need to do this if there are more than one. (bladerf=00001 for BladeRF with serial 00001 or rtl=00923838 for RTL-SDR with serial 00923838, just airspy for an airspy)
 - **system** - This object defines the trunking system that will be recorded
   - **control_channels** - an array of the control channel frequencies for the system, in Hz. Right now, only the first value is used.
   - **type** - the type of trunking system. The options are *smartnet* & *p25*.
   - **modulation** - the type of modulation that the system uses. The options are *QPSK* & *FSK4*.
   - **captureDir** - the complete path to the directory where recordings should be saved. 
 - **talkgroupsFile** - this is a CSV file that provides information about the talkgroups. It determines whether a talkgroup is analog or digital, and what priority it should have. 
 - **defaultMode** - Default mode to use when a talkgroups is not listed in the **talkgroupsFile** [digital/analog].

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

To user mutliple SDRs, simply define additional Sources in the Source array. The `confing-multi-rtl.json.sample` has an example of how to do this. In order to tell the different SDRs apart and make sure they get the right error correction value, give them a serial number using the `rtl_eeprom -s` command and then specifying that number in the `device` setting for that Source.

###How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording.





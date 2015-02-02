Trunk Recorder
=================

Trunk Recorder is able to record the calls on a trunked radio system. It uses 1 or more Software Defined Radios (SDRs) to do. The SDRs capture large swatches of RF and then use software to process what was recieved. GNURadio is used to do this processing and provides lots of convienent RF blocks that can be pieced together to do complex RF processing. Right now it can only record one Trunked System at a time.

Trunk Recorder currently supports the following:
 - P25 & SmartNet Trunking Systems
 - SDRs that use the OsmoSDR source ( HackRF, RTL - TV Dongles, BladeRF, and more)
 - Ettus USRP
 - P25 Phase 1 & Analog voice
 
I have tested things on both Unbuntu 14.04 & OSX 10.10. I have been using it with an Ettus b200, and a HackRF Jawbreaker.

##Compile

###Requirements
 - GNURadio 3.7
 - GR-DSD (The version I forked)
 - OP-25
  
*GNURadio*
It is important to have a very recent version of GnuRadio (GR). There was a bug in earlier versions that messed up the smartnet trunking. Make sure your install is up to date if you are having trouble deoding smartnet trunking.

If you are running Linux, the easiest way to install GR is by using [Pybombs](http://gnuradio.org/redmine/projects/pybombs/wiki/QuickStart). After you have installed using pybombs, make sure you setup you Environment variables. In your pybombs directory, run: `./pybombs env` and then load them `source $prefix/setup_env.sh`, with $prefix being the directory you installed GR in.

If you are on OSX, the [MacPorts](https://gnuradio.org/redmine/projects/gnuradio/wiki/MacInstall) install has worked for me.

*GR-DSD*
I made a fork of this code to allow for more statistics to be collected and the also make usre multiple copies can run at once. It has a few dependencies:
 - Lib Snd File: `sudo apt-get install libsndfile`
 - ITPP: `sudo apt-get install libitpp-dev`

Now download, compile and install the code. Make sure you have loaded the environment variables that point to the GR libaries first.
```
git clone https://github.com/robotastic/gr-dsd.git
cd gr-dsd
cmake -DCMAKE_PREFIX_PATH=/path/to/GR/install   .
make
sudo make install
sudo ldconfig
```
Note - if you did not install GR in the standard spot, use the -DCMAKE_PREFIX_PATH to point to it.

*OP25*
This should be as simple doing `./pybombs install gr-op25`. On OSX, it is quite a bit trickier. Right now the code is not patched for OSX installs. I will try to put together an easy way to do an OSX install in the future.

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

###How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording.





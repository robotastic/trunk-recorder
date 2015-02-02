Trunk Recorder
=================

Trunk Recorder is able to record the calls on a trunked radio system. It uses 1 or more Software Defined Radios (SDRs) to do. The SDRs capture large swatches of RF and then use software to process what was recieved. GNURadio is used to do this processing and provides lots of convienent RF blocks that can be pieced together to do complex RF processing.

Trunk Recorder currently supports the following:
 - P25 & SmartNet Trunking System
 - SDRs that use the OsmoSDR source ( HackRF, RTL - TV Dongles, BladeRF, and more)
 - Ettus USRP
 - P25 Phase 1 & Analog voice
 
I have tested things on both Unbuntu 14.04 & OSX 10.10. I have been using it with an Ettus b200, and a HackRF Jawbreaker.




###How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording.





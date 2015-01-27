smartnet-recorder
=================

Uses a HackRF to record all of the radio transmissions on a SmartNet II system.

After a bit of work, I have put together a system that lets you monitor the radio system for DC Fire, EMS & City Services. Everything gets recorded and can be played back through a website. Thanks to the magic websockets, any new call that comes in gets added to the top of the list. Better yet, if you hit the Autoplay button in the upper left hand corner, it will automatically play through the list of calls. You can narrow the list of calls display to specific group using the filters.

Anyhow, give it a try at openmhz.com and let me know what you think. If you want more background on how it works, read on…

###HackRF
Software Defined Radios are pretty awesome. For the uninitiated, they let you receive a lot of radio signal over a wide range of frequencies. It pass you the raw information and you use a computer to filter out a transmission and process the signal. There is tons of flexibility… and that is also a challenge. There are a lot of rough edges and ways you can mess things up. That is half the fun though, boldly coding where few have coded before.

I was one of the lucky recipients of a great SDR, the HackRF Jawbreaker. It is an amazing piece of Hardware, capable of sending or receiving 20MHz of spectrum between ~13MHz – 6GHz. It is Open Source Hardware too, so if you are wicked smart you can build your own boards. The design work for it was funded by DARPA and I got one of the boards from that pre-production run. In order to go into production, a Kickstarter project was put together. The original target amount was $80k and it quickly blew through that and ended up raising $600k. One of the most impressive stats is that the target price is $300.

You can do a lot of things with this board. It use the popular Osmosdr drivers, so it works with a lot of existing things and plugs right into GNURadio.

There are an endless number of things to try. What I have been focusing on is trying to monitor the radio system that the Washington, DC Fire/EMS & City Services use. Of course monitoring a radio system is nothing new. Radio Shack sells a bunch of different scanners that can do it. However, these scanners can only follow one conversation on a system on a time. Since an SDR can receive a wide swatch a spectrum at once and all the processing happens on the computer, you can decode multiple transmissions. Since you are doing the processing on a computer you can easily save and archive the transmissions, which you sort of needed since you could be getting a couple at once.

Luckily for me, a couple of people have already setup systems that do exactly that. The code from Nick Foster, GR-SmartNet, seems to be the first out there, and the only publicly available code. The Super Trunking Scanner took it a step further and made it playable over the web. It monitor a trunked system with analog channels. The Radio Capture system took a similar approach, except made it work for a system with digital voice channels.

###How Trunking Works
Here is a little background on trunking radio systems, for those not familiar. In a Trunking system, one of the radio channels is set aside for to manage the assignment of radio channels to talkgroups. When someone wants to talk, they send a message on the control channel. The system then assigns them a channel and sends a Channel Grant message on the control channel. This lets the talker know what channel to transmit on and anyone who is a member of the talkgroup know that they should listen to that channel.

In order to follow all of the transmissions, this system constantly listens to and decodes the control channel. When a channel is granted to a talkgroup, the system creates a monitoring process. This process will start to process and decode the part of the radio spectrum for that channel which the SDR is already pulling in. In the DC system, the audio is digitally encoded using the P25 CAI process. Decoding it is a bit of pain. I am taking a quick and dirty approach right now and have shoe horned in the DSD program. In the future I would like to try using the code from the OP25 project and a hardware dongle for the decoding. Unfortunately, the dongle is $500, so that might not be happening too soon.

No message is transmitted on the control channel when a talkgroup’s conversation is over. So instead the monitoring process keeps track of transmissions and if there has been no activity for 5 seconds, it ends the recording and uploads to the webserver. I convert the WAV file that gets recorder into an MP3 file. Since the audio is original converted to digital by the radio system, put it through another lossy digital conversion is probably not a good idea, but sending the full-size WAV file ate up too much space.

###My Setup
The monitoring and recording is being run off of a laptop in my apartment and uses a crappy antenna. The website is run off a VPS I have running up in the magical cloud.

The webserver is pretty simple. It is written in NodeJs. The audio is stored as WAV files and indexed using MongoDB. The server simply watches for new files being placed in a directory and then moves them and adds them to the DB. Socket.io is used to updated all of the browsers visiting the site that a new transmission has been added.

###The Code
The recorder portion of the system is C++ code that uses GnuRadio 3.7.4

The recorder uses DSD to decode the digital audio. Unfortunately DSD isn’t supported or being developed, isn’t designed to work with GNURadio or SDR. Luckily someone wrapped DSD into a GNURadio block. It works, but it isn’t pretty. I had to futz with it a bit to run concurrently. My version is here.

The final portion is the website for listening to the recordings. The code for that is available here.



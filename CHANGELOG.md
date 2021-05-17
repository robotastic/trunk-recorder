Trunk Recorder ChangeLog
========================

### Version 3.0.1
* Updated to the latest version of OP25. Supposed performance improvements.

### Version 3.0
*I really have to do a better job of tracking changes... I think I got most items.*
* Bug fixes on Phase 2 decoding, catching some additional messages.
* Sending status over a WebSocket

### Version 2.2.1
* Just capturing random bug fixes

### Version 2.2
Lot's of fun things added!
* P25 Phase 2 support was added. It should just work and the P25 recorder should automatically switch between Phase 1 and Phase 2 depending on what it sees on the trunking channel.
* SmartNet BandPlan support - lets you set configs in the config.json file to specify which type of bandplan to use or define a custom one. If anyone is using VHF systems, let me know... work it probably still needed.
* Error logging - P25 recorders now keep tracking of the number of errors during decoded for each call recording. This is spit out in the Freq List in the JSON file and uploaded to openMHz.
* audioArchive - config lets you select if you don't want to save the audio recordings after they are uploaded.
* callLog - config lets you turn off JSON file creation.
* logs - I changed the formatting of the logs that are printed to the screen.
* Specify the config file from the command line using the `--config=x.json` argument.

### Version 2.1.2
* Added support for Conventional Analog and P25 systems
* Improved FSK4 performance

### Version 2.0
* Adds support for recording multiple systems and switches to using FFT for recorders.

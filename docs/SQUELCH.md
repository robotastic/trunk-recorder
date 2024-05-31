# Adjusting Squelch for Conventional Systems

flowchart TD
 subgraph subgraph_5wys65oxs["Conventional Recorder"]
        node_9tex2m50c["Squelch"]
  end
    A("SDR Source") --> B("Signal Detector") & C("Recorder Selector")
    B -. Turn On Recorder .- C
    C --> subgraph_5wys65oxs
    B -.- nk["Automatic Signal Detection Threshold"]
    style nk stroke-width:2px,stroke-dasharray: 2

Conventional can use up a lot of CPU. Prior to v5.0, conventional recorders are always "on" waiting for a signal to break squelch. With v5.0 you now have a signal detector per source. its job is to look over the bandwidth for that source. It performs an FFT and for each bin that is over a "threshold" it will look for a conventional channel that lines up with it. If it finds a channel, it will start sending samples to it, effectively turning it on. In your logs, look for "blah blah freq Enabled" if you see that, it means that the signal dectector has turned on a conventional recorder. The Signal Detector automatically tries to find this threshold. it is not perfect. If you are seeing a recorder not turn on when there is a signal, that could be because the Signal Detector threshold is not set correctly.... but the auto detect is prob getting messed up by the AGC. maybe.

There is also the squelch setting. it waits for a signal go above a certain level and for analog, then writes it to disk. If you are getting static, it is because the squelch value is too high. 

Things to try:

- Use GQRX to fine tune gain. Turn off AGC
- Set Signal Detector Threshold to something high like -10 so that recorders are always enabled.
- Set sequelch value for the recorders so that it starts and stop predictably
- remove Signal Detector Threshold and see if the automatic setting works
- if auto doesn't work, fine tune the Signal Detector threshold so that it only displays the Enabled when a recording starts

I will try to write up some guide on doing this
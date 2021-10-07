# Ugh! Why is it doing that thing

## Common Error Messages and What to Do

### gr::log :DEBUG: correlate_access_code_tag_bb0

These messages come from GnuRadio. You can change the [default level of logging](https://wiki.gnuradio.org/index.php/Logging) for GnuRadio to avoid them. To make the change system wide, edit: **/etc/gnuradio/conf.d/gnuradio-runtime.conf** and edit the log level

```bash
log_level = info 
```


To edit at the user level, edit/create: **"~/.gnuradio/config.conf** and add this line:

```bash
log_level = info 
```

### I am receiving control messages, but I am only getting empty recordings

Generally this is a configuration issue. Try setting `controlWarnRate` to 0, in the top level of the config.json file. This will tell you how many control channel messages you are actually receiving. It should be around 30 or 40 per sec. It may also be worth checking to see if you have the modulation correct. Even with TCXO, sometimes there can be a little error and the voice channels can be more sensitive than the control channel. The other knob is the gain, make sure it is adjust correctly. Finally, unless you are recording Analog, remove any squelch settings.




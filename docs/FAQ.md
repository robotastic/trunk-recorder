---
sidebar_label: 'FAQ'
sidebar_position: 7
---

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

### It is crashing!

It will take a little bit of work to figure out what part of the Trunk Recorder is crashing. Linux has some built in tools to help with this. When a crash happens, a copy of the programs memory can be written to disk in a **core** file. This is known as a core dump. It will be written to the directory the program was started from.

The first step is to remove any old **core** files:

`rm core`

Next set the size limit for the **core** file to be unlimited:

`ulimit -c unlimited`

Now start Trunk Recorder and do whatever it takes to make it crash. This should generate a **core** file. You can the use **gdb** to inspect the **core**:

`gdb trunk-recorder core`

Once **gdb** loads up, use the *backtrace* command to review the execution history prior to the crash:

`backtrace`

Now you can create a [GitHub Issue](https://github.com/robotastic/trunk-recorder/issues), including the info from **gdb**, along with your config.json and as much info as possible on how to recreate the crash.


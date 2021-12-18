# How to Debuga Seg Fault

When Trunk Recorder crashes and give a Segmentation Fault error (a SegFault), there are some steps you can take to figure out what is causing it. Pulling this information makes it a lot easier to figure what is going wrong.

1. In the terminal where you are going to be running Trunk Recorder, change the limit on Core Dumps, which lets Linux record what was happening at the time of the crash:
```bash
ulimit -c unlimited
```

2. Next, rebuild Trunk Recorder with extra debug information information include. To do this, add `-DCMAKE_BUILD_TYPE=Debug` to the **cmake** command as part of the build process. After you do that, run `make` again. For example, if your build directory was in the Trunk Recorder directory, run:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
```

3. Before you start Trunk Recorder, delete any old **core** files that may be in the directory.

4. Startup Trunk Recorder and wait for a crash...

5. After a crash, use the **gdb** program to unpack the **core** file that was generated during the crash.
```bash
gdb trunk-recorder core
```

6. *gdb* is a powerful debugging platform. However, all we need is a trace of the crash. After *gdb* has finished loading, type in `bt` to get a trace. Copy all of the output from *gdb* into a new [GitHub Issue](https://github.com/robotastic/trunk-recorder/issues/new), along with as much information as possible on what maybe casuing the crash.



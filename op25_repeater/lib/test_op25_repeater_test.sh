#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/Users/luke/Programming/trunk-recorder/op25_repeater/lib
export GR_CONF_CONTROLPORT_ON=False
export PATH=/Users/luke/Programming/trunk-recorder/op25_repeater/lib:$PATH
export DYLD_LIBRARY_PATH=/Users/luke/Programming/trunk-recorder/op25_repeater/lib:$DYLD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-op25_repeater 

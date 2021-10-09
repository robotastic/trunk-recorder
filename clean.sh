#!/bin/bash
rm -R ./CMakeFiles
rm    ./cmake_install.cmake
rm    ./CMakeCache.txt
if [ "$1" == "cleanlib" ]; then
   rm -R ./lib/op25_repeater/CMakeFiles
   rm    ./lib/op25_repeater/cmake_install.cmake
   rm -R ./lib/op25_repeater/include/op25_repeater/CMakeFiles
   rm    ./lib/op25_repeater/include/op25_repeater/cmake_install.cmake
   rm -R ./lib/op25_repeater/lib/CMakeFiles
   rm    ./lib/op25_repeater/lib/cmake_install.cmake
   rm -R ./lib/op25_repeater/lib/imbe_vocoder/CMakeFiles
   rm    ./lib/op25_repeater/lib/imbe_vocoder/cmake_install.cmake
fi
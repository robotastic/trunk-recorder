INCLUDE(FindPkgConfig)
if(NOT LIBOP25_FOUND)
  pkg_check_modules (LIBOP25_PKG libop25)
  find_path(LIBOP25_INCLUDE_DIR
    NAMES op25/api.h
    HINTS ${LIBOP25_PKG_INCLUDE_DIRS}
    PATHS /usr/include
          /usr/local/include
          /home/luke/gr3.7/include
  )

  find_library(LIBOP25_LIBRARIES
    NAMES gnuradio-op25
    HINTS ${LIBOP25_PKG_LIBRARY_DIRS}
    PATHS /usr/lib
          /usr/local/lib
          /home/luke/gr3.7/lib
  )
message(STATUS "Pkg: ${LIBOP25_PKG}, ${LIBOP25_PKG_INCLUDE_DIRS}, ${LIBOP25_PKG_LIBRARY_DIRS}")
message(STATUS "Vars: ${LIBOP25_INCLUDE_DIR}, ${LIBOP25_LIBRARIES}")
if(LIBOP25_INCLUDE_DIR AND LIBOP25_LIBRARIES)
  set(LIBOP25_FOUND TRUE CACHE INTERNAL "libop25 found")
  message(STATUS "Found libop25: ${LIBOP25_INCLUDE_DIR}, ${LIBOP25_LIBRARIES}")
else(LIBOP25_INCLUDE_DIR AND LIBOP25_LIBRARIES)
  set(LIBOP25_FOUND FALSE CACHE INTERNAL "libop25 found")
  message(STATUS "libop25 not found.")
endif(LIBOP25_INCLUDE_DIR AND LIBOP25_LIBRARIES)

mark_as_advanced(LIBOP25_INCLUDE_DIR LIBOP25_LIBRARIES)

endif(NOT LIBOP25_FOUND)

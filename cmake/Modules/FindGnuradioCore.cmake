INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_GNURADIO_CORE gnuradio-core)

FIND_PATH(
    GNURADIO_CORE_INCLUDE_DIRS
    NAMES gr_core_api.h
    HINTS $ENV{GNURADIO_CORE_DIR}/include/gnuradio
        ${PC_GNURADIO_CORE_INCLUDEDIR}
        ${CMAKE_INSTALL_PREFIX}/include/gnuradio
    PATHS /usr/local/include/gnuradio
          /usr/include/gnuradio
)

FIND_LIBRARY(
    GNURADIO_CORE_LIBRARIES
    NAMES gnuradio-core
    HINTS $ENV{GNURADIO_CORE_DIR}/lib
        ${PC_GNURADIO_CORE_LIBDIR}
        ${CMAKE_INSTALL_PREFIX}/lib64
        ${CMAKE_INSTALL_PREFIX}/lib
    PATHS /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GNURADIO_CORE DEFAULT_MSG GNURADIO_CORE_LIBRARIES GNURADIO_CORE_INCLUDE_DIRS)
MARK_AS_ADVANCED(GNURADIO_CORE_LIBRARIES GNURADIO_CORE_INCLUDE_DIRS)

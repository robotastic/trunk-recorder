INCLUDE(FindPkgConfig)
if(NOT LIBOP25_FOUND)
  pkg_check_modules (LIBOP25_PKG libop25)
  find_path(LIBOP25_INCLUDE_DIR
    NAMES op25/api.h
    HINTS $ENV{GNURADIO_RUNTIME_DIR}/include
          ${PC_GNURADIO_RUNTIME_INCLUDE_DIRS}
          ${CMAKE_INSTALL_PREFIX}/include
    PATHS /usr/local/include
          /usr/include
  )

  find_library(LIBOP25_LIBRARIES
    NAMES gnuradio-op25
    HINTS $ENV{GNURADIO_RUNTIME_DIR}/lib
          ${PC_GNURADIO_RUNTIME_LIBDIR}
          ${CMAKE_INSTALL_PREFIX}/lib/
          ${CMAKE_INSTALL_PREFIX}/lib64/
    PATHS /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
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

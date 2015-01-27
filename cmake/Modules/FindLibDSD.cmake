INCLUDE(FindPkgConfig)
if(NOT LIBDSD_FOUND)
  pkg_check_modules (LIBDSD_PKG libdsd)
  find_path(LIBDSD_INCLUDE_DIR
    NAMES dsd/dsd_api.h
    HINTS ${LIBDSD_PKG_INCLUDE_DIRS}
    PATHS /usr/include
          /usr/local/include
  )

  find_library(LIBDSD_LIBRARIES
    NAMES gr-dsd
    HINTS ${LIBDSD_PKG_LIBRARY_DIRS}
    PATHS /usr/lib
          /usr/local/lib
  )
message(STATUS "Pkg: ${LIBDSD_PKG}, ${LIBDSD_PKG_INCLUDE_DIRS}, ${LIBDSD_PKG_LIBRARY_DIRS}")
message(STATUS "Vars: ${LIBDSD_INCLUDE_DIR}, ${LIBDSD_LIBRARIES}")
if(LIBDSD_INCLUDE_DIR AND LIBDSD_LIBRARIES)
  set(LIBDSD_FOUND TRUE CACHE INTERNAL "libdsd found")
  message(STATUS "Found libdsd: ${LIBDSD_INCLUDE_DIR}, ${LIBDSD_LIBRARIES}")
else(LIBDSD_INCLUDE_DIR AND LIBDSD_LIBRARIES)
  set(LIBDSD_FOUND FALSE CACHE INTERNAL "libdsd found")
  message(STATUS "libdsd not found.")
endif(LIBDSD_INCLUDE_DIR AND LIBDSD_LIBRARIES)

mark_as_advanced(LIBDSD_INCLUDE_DIR LIBDSD_LIBRARIES)

endif(NOT LIBDSD_FOUND)

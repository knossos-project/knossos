# QUAZIP_FOUND               - QuaZip library was found
# QUAZIP_INCLUDE_DIRS        - Path to QuaZip and zlib
# QUAZIP_LIBRARIES           - List of QuaZip libraries

if(QUAZIP_INCLUDE_DIRS AND QUAZIP_LIBRARIES)
    # in cache already
    set(QUAZIP_FOUND TRUE)
else(QUAZIP_INCLUDE_DIRS AND QUAZIP_LIBRARIES)
    find_package(PkgConfig)
    find_package(ZLIB REQUIRED)

    set(QUAZIP_HINTS "C:/Programme/" "C:/Program Files")
    find_library(QUAZIP_LIBRARY NAMES quazip HINTS ${QUAZIP_HINTS} PATH_SUFFIXES "QuaZip")
    find_path(QUAZIP_INCLUDE_DIR NAMES quazip/quazip.h HINTS ${QUAZIP_HINTS})

    set(QUAZIP_LIBRARIES ${QUAZIP_LIBRARY} ${ZLIB_LIBRARIES})
    set(QUAZIP_INCLUDE_DIRS ${QUAZIP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

    set(QUAZIP_VERSION 0.7.0)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(QUAZIP
        REQUIRED_VARS QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIRS
        VERSION_VAR QUAZIP_VERSION
    )
endif(QUAZIP_INCLUDE_DIRS AND QUAZIP_LIBRARIES)

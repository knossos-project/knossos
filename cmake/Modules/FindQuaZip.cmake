# provides an imported target for the quazip library

find_package(ZLIB REQUIRED)

find_library(QUAZIP_LIBRARY NAMES quazip quazip5 PATH_SUFFIXES "QuaZip")
find_path(QUAZIP_INCLUDE_DIR quazip.h PATH_SUFFIXES quazip quazip5)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QUAZIP
    REQUIRED_VARS QUAZIP_LIBRARY QUAZIP_INCLUDE_DIR
)

if(QUAZIP_FOUND)
    add_library(QuaZip::QuaZip UNKNOWN IMPORTED)
    set_target_properties(QuaZip::QuaZip PROPERTIES
        IMPORTED_LOCATION ${QUAZIP_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${QUAZIP_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
    )
    if(NOT ${QUAZIP_LIBRARY} MATCHES ".so" AND NOT ${QUAZIP_LIBRARY} MATCHES ".dll")
    set_target_properties(QuaZip::QuaZip PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "QUAZIP_STATIC"
    )
    endif()
endif(QUAZIP_FOUND)

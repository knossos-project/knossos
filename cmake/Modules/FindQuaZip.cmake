# provides an imported target for the statically built quazip library

find_package(ZLIB REQUIRED)

find_library(QUAZIP_LIBRARY quazip PATH_SUFFIXES "QuaZip")
find_path(QUAZIP_INCLUDE_DIR quazip/quazip.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QUAZIP
    REQUIRED_VARS QUAZIP_LIBRARY QUAZIP_INCLUDE_DIR
)

if(QUAZIP_FOUND)
    add_library(QuaZip::QuaZip STATIC IMPORTED)
    set_target_properties(QuaZip::QuaZip PROPERTIES
        IMPORTED_LOCATION ${QUAZIP_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${QUAZIP_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES ${ZLIB_LIBRARIES}
        INTERFACE_COMPILE_DEFINITIONS "QUAZIP_STATIC"
    )
endif(QUAZIP_FOUND)

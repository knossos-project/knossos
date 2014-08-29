# provides an imported target for the dynamically built snappy library

find_library(SNAPPY_LIB snappy)
find_path(SNAPPY_INCLUDE snappy.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNAPPY
    REQUIRED_VARS SNAPPY_LIB SNAPPY_INCLUDE
)

if(SNAPPY_FOUND)
    add_library(Snappy::Snappy SHARED IMPORTED)
    set_target_properties(Snappy::Snappy PROPERTIES
        IMPORTED_LOCATION ${SNAPPY_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${SNAPPY_INCLUDE}
    )
endif(SNAPPY_FOUND)

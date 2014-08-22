# provides an imported target for the TurboJPEG library

find_library(TURBOJPEG_LIBRARY turbojpeg)
find_path(TURBOJPEG_INCLUDE_DIR turbojpeg.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TURBOJPEG DEFAULT_MSG TURBOJPEG_LIBRARY TURBOJPEG_INCLUDE_DIR)

if(TURBOJPEG_FOUND)
    add_library(TurboJPEG::TurboJPEG STATIC IMPORTED)
    set_target_properties(TurboJPEG::TurboJPEG PROPERTIES
        IMPORTED_LOCATION ${TURBOJPEG_LIBRARY}
        INTERFACE_COMPILE_DEFINITIONS "KNOSSOS_USE_TURBOJPEG"#enable turbojpeg inside knossos
        INTERFACE_INCLUDE_DIRECTORIES ${TURBOJPEG_INCLUDE_DIR}
    )
endif(TURBOJPEG_FOUND)
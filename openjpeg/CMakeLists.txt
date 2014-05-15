#build openjpeg always in release mode,
#even if the main project is built in debug mode
SET(${PROJECT_NAME}_OPENJPEG_C_FLAGS "-O3" CACHE STRING "${PROJECT_NAME} OPENJPG C FLAGS")
SET(CMAKE_C_FLAGS "${${PROJECT_NAME}_OPENJPEG_C_FLAGS}")

SET(OPENJPG_SOURCES
    cio.c
    bio.c
    color.c
    convert.c
    dwt.c
    event.c
    function_list.c
    image.c
    index.c
    invert.c
    j2k.c
    jp2.c
    mct.c
    mqc.c
    openjpeg.c
    opj_clock.c
    opj_decompress.c
    opj_getopt.c
    pi.c
    raw.c
    t1.c
    t2.c
    tcd.c
    tgt.c
)
    
add_library(openjpeg_knossos
    ${OPENJPG_SOURCES}
)
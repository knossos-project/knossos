execute_process(
    COMMAND git describe --always --dirty --tags
    OUTPUT_VARIABLE KREVISION
    WORKING_DIRECTORY ${GIT}
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(${SRC} ${DST} @ONLY)#substitute revision in version.h.in

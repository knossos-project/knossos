execute_process(
    COMMAND git describe --always --dirty --tags
    OUTPUT_VARIABLE KREVISION
    WORKING_DIRECTORY ${GIT}
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git log -1 --format=%aI
    OUTPUT_VARIABLE KREVISIONDATE
    WORKING_DIRECTORY ${GIT}
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(${SRC} ${DST} @ONLY)

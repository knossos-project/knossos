execute_process(
    COMMAND git describe --always --dirty --tags
    OUTPUT_VARIABLE KREVISION
    WORKING_DIRECTORY ${GIT}
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP KBUILDDATE %Y-%m-%d UTC)
configure_file(${SRC} ${DST} @ONLY)

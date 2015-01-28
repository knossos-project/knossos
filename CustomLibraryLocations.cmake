#specify extraordinary locations for convenience here (forward slashes essential)
#cmake searches for headers and libraries both directly and inside include/lib folders below
set(CMAKE_PREFIX_PATH
    "${CMAKE_PREFIX_PATH}"#append

# For Mac development: required to find packages from Homebrew
    "/usr/local/opt/curl/bin"
    "/usr/local/opt/qt5/bin"
)

if(WIN32 AND NOT BUILD_SHARED_LIBS)
    set(CMAKE_PREFIX_PATH
        "${CMAKE_PREFIX_PATH}"
        "C:/msys64/mingw64/qt5-static"
    )
endif()

#specify extraordinary locations for convenience here (forward slashes essential)
#cmake searches for headers and libraries both directly and inside include/lib folders below
set(CMAKE_PREFIX_PATH
    "${CMAKE_PREFIX_PATH}"#append

# For Mac development: required to find packages from Homebrew
    "/usr/local/opt/curl/bin"
    "/usr/local/opt/qt5/bin"
)

if(WIN32 AND NOT BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "x64 static build")
    set(CMAKE_PREFIX_PATH
        "${CMAKE_PREFIX_PATH}"
        "C:/msys64/mingw64/qt5-static"
        "C:/dev/curl-7.40.0/_build_64/_install"
    )
elseif(WIN32 AND NOT BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "x32 static build")
    set(CMAKE_PREFIX_PATH
        "${CMAKE_PREFIX_PATH}"
        "C:/msys64/mingw32/qt5-static"
        "C:/dev/curl-7.40.0/_build_32/_install"
    )
endif()

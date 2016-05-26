#specify extraordinary locations for convenience here (forward slashes essential)
#cmake searches for headers and libraries both directly and inside include/lib folders below
set(CMAKE_PREFIX_PATH
    "${CMAKE_PREFIX_PATH}"#append
    "/home/void/stuff/repo/PythonQt/bin/install"

# For Mac development: required to find packages from Homebrew
    "/usr/local/opt/qt5/bin"
)

# find static qt libs (default msys2 location), WD should be /usr/bin or /mingw??/bin
if(WIN32 AND NOT BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "x64 static build")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{WD}/../../mingw64/qt5-static/")
elseif(WIN32 AND NOT BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "x32 static build")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{WD}/../../mingw32/qt5-static/")
endif()

# find system python dll
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SystemRoot}/System32")
elseif(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{SystemRoot}/SysWOW64")
endif()

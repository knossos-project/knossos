#specify extraordinary locations for convenience here
#cmake searches for headers and libraries both directly and inside include/lib folders below
set(CMAKE_PREFIX_PATH
    "${CMAKE_PREFIX_PATH}"#append
    "E:/dev/qt/Qt-5.2.1-x86_64/lib/cmake"#not needed if qmake.exe is in path
    "E:/dev/curl-7.36.0/_install"
    "E:/dev/freeglut-2.8.1/_install"
    "C:/libjpeg-turbo-gcc"
    "C:/libjpeg-turbo-gcc64"
)
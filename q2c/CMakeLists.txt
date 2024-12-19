#-----------------------------------------------------------------
# Project converted from qmake file using q2c
# https://github.com/benapetr/q2c at Wed Dec 18 08:46:21 2024
#-----------------------------------------------------------------
cmake_minimum_required (VERSION 3.0)
project(q2c)
option(QT5BUILD "Build using Qt5 libs" TRUE)
set(QT_QMAKE_EXECUTABLE "D:/Qt/Qt5.0.12/5.12.0/mingw73_64/bin/qmake.exe")
set(Qt5_DIR "D:/Qt/Qt5.12.0/5.12.0/mingw73_64/lib/cmake/Qt5")
set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.12.0/5.12.0/mingw73_64")
IF (QT5BUILD)
    find_package(Qt5Core REQUIRED)
    set(QT_INCLUDES ${Qt5Core_INCLUDE_DIRS})
    include_directories(${QT_INCLUDES})
    QT5_WRAP_CPP(q2c_HEADERS_MOC ${HuggleLite_HEADERS})
ELSE()
    find_package(Qt4 REQUIRED)
ENDIF()
set(q2c_SOURCES "main.cpp" "terminalparser.cpp" "configuration.cpp" "project.cpp" "logs.cpp" "generic.cpp")
set(q2c_HEADERS "terminalparser.h" "configuration.h" "project.h" "logs.h" "generic.h")
add_executable(q2c ${q2c_SOURCES} ${q2c_HEADERS})
IF (QT5BUILD)
    qt5_use_modules(q2c Core)
ENDIF()

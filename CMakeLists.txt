cmake_minimum_required(VERSION 3.16)
project(QtVirtualKeyboard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(LayerShellQt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_executable(QtVirtualKeyboard
    main.cpp
    VirtualKeyboard.cpp
    VirtualKeyboard.h
)

# Tell the compiler to look in the build directory for the generated header
target_include_directories(QtVirtualKeyboard PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
include_directories(${XKBCOMMON_INCLUDE_DIRS})

target_link_libraries(QtVirtualKeyboard
    Qt6::Widgets
    Qt6::Multimedia
    LayerShellQt::Interface
	${XKBCOMMON_LIBRARIES}
)
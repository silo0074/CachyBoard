cmake_minimum_required(VERSION 3.16)
project(QtVirtualKeyboard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WaylandClient)
find_package(LayerShellQt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(X11 REQUIRED x11 xtst)

# Find the scanner tool
find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)

# Point to the file you downloaded in the project root
set(VIRTUAL_KEYBOARD_XML "${CMAKE_CURRENT_SOURCE_DIR}/virtual-keyboard-unstable-v1.xml")

# Verify the file exists before proceeding to avoid cryptic Ninja errors
if(NOT EXISTS "${VIRTUAL_KEYBOARD_XML}")
    message(FATAL_ERROR "XML file not found at ${VIRTUAL_KEYBOARD_XML}. Please ensure you ran the curl command in the project root.")
endif()

# Define generated file paths in the BUILD directory
set(GEN_HEADER "${CMAKE_CURRENT_BINARY_DIR}/wayland-virtual-keyboard-unstable-v1-client-protocol.h")
set(GEN_CODE "${CMAKE_CURRENT_BINARY_DIR}/wayland-virtual-keyboard-unstable-v1-protocol.c")

set(IM_GEN_HEADER "${CMAKE_CURRENT_BINARY_DIR}/wayland-input-method-unstable-v1-client-protocol.h")
set(IM_GEN_CODE "${CMAKE_CURRENT_BINARY_DIR}/wayland-input-method-unstable-v1-protocol.c")
set(INPUT_METHOD_XML "/usr/share/wayland-protocols/unstable/input-method/input-method-unstable-v1.xml")

add_custom_command(
    OUTPUT "${IM_GEN_HEADER}" "${IM_GEN_CODE}"
    COMMAND ${WAYLAND_SCANNER} client-header "${INPUT_METHOD_XML}" "${IM_GEN_HEADER}"
    COMMAND ${WAYLAND_SCANNER} private-code "${INPUT_METHOD_XML}" "${IM_GEN_CODE}"
    DEPENDS ${INPUT_METHOD_XML}
)

add_custom_command(
    OUTPUT "${GEN_HEADER}" "${GEN_CODE}"
    COMMAND ${WAYLAND_SCANNER} client-header "${VIRTUAL_KEYBOARD_XML}" "${GEN_HEADER}"
    COMMAND ${WAYLAND_SCANNER} private-code "${VIRTUAL_KEYBOARD_XML}" "${GEN_CODE}"
    DEPENDS "${VIRTUAL_KEYBOARD_XML}"
    VERBATIM
)

add_executable(QtVirtualKeyboard
    main.cpp
    VirtualKeyboard.cpp
    VirtualKeyboard.h
    "${GEN_HEADER}"
    "${GEN_CODE}"
    "${IM_GEN_HEADER}"
    "${IM_GEN_CODE}"
)

# Tell the compiler to look in the build directory for the generated header
target_include_directories(QtVirtualKeyboard PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(QtVirtualKeyboard
    Qt6::Widgets
    Qt6::WaylandClient
    LayerShellQt::Interface
    ${WAYLAND_CLIENT_LIBRARIES}
    ${X11_LIBRARIES}
)
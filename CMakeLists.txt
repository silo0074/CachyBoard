cmake_minimum_required(VERSION 3.16)

# Project info
project(CachyBoard 
    VERSION 1.0.0 
    LANGUAGES CXX
)
set(PROJECT_RELEASE 1) # package version

# Create a lowercase variable
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
# Result: PROJECT_NAME_LOWER = "cachyboard"

# Pass the version to C++ as a macro
add_definitions(-DAPP_VERSION="${PROJECT_VERSION}")
add_definitions(-DAPP_NAME="${PROJECT_NAME}")

# Adds the flag to all targets in the project
add_compile_options(-fstack-protector-strong)

# Add it to the linker
add_link_options(-fstack-protector-strong)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON) # Automatically run MOC
set(CMAKE_AUTOUIC ON) # Automatically run UIC (if you use .ui files later)
set(CMAKE_AUTORCC ON) # Automatically run RCC (if you use .qrc files)

# Get project source directory
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(LayerShellQt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_executable(${PROJECT_NAME}
    main.cpp
    VirtualKeyboard.cpp
    VirtualKeyboard.h
	resources.qrc
)

# Tell the compiler to look in the build directory for the generated header
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
include_directories(${XKBCOMMON_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    Qt6::Multimedia
    LayerShellQt::Interface
	${XKBCOMMON_LIBRARIES}
)

# --- INSTALLATION ---
include(GNUInstallDirs)

# Install the Executable
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the .desktop file
install(FILES "assets/${PROJECT_NAME_LOWER}.desktop" 
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
)

# Install the Icon
install(FILES "assets/${PROJECT_NAME_LOWER}.svg" 
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps"
)

# --- CPACK CONFIGURATION ---
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Liviu Istrate")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_RELEASE ${PROJECT_RELEASE})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/silo0074/CachyBoard")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "On-screen virtual keyboard for Wayland.")
set(CPACK_PACKAGE_CONTACT "silo0074@github.com") # Required for DEB

# Architecture logic
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(CPACK_SYSTEM_NAME "x86_64")
else()
    set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Force the filename format for DEB and RPM
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PROJECT_RELEASE}-${CPACK_SYSTEM_NAME}")

# Specific for Debian to follow convention (all lowercase often preferred)
set(CPACK_DEBIAN_PACKAGE_NAME "${PROJECT_NAME_LOWER}")

set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.spec")
set(CPACK_RPM_PACKAGE_RELEASE ${PROJECT_RELEASE})

set(CPACK_DEBIAN_PACKAGE_RELEASE ${PROJECT_RELEASE})
set(CPACK_DEBIAN_PACKAGE_LICENSE "GPL-3.0")



# --- DEPENDENCIES ---

# For DEB (Ubuntu/Debian)
# Note: xkbcommon is libxkbcommon0 on Debian
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6, libqt6multimedia6, layershell-qt, libxkbcommon0")

# For RPM (Fedora/OpenSUSE)
# Fedora uses qt6-qtbase-gui; OpenSUSE uses qt6-widgets. Using names that satisfy both:
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-base, qt6-multimedia, layer-shell-qt, libxkbcommon")

include(CPack)
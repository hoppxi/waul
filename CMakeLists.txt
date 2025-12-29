cmake_minimum_required(VERSION 3.10)
project(waul CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WLR_PROTOCOLS REQUIRED wlr-protocols)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(STB REQUIRED stb)

# Protocol Paths
execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=pkgdatadir wlr-protocols OUTPUT_VARIABLE WLR_PROTOCOLS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
find_program(WAYLAND_SCANNER NAMES wayland-scanner)

set(LAYER_SHELL_XML "${WLR_PROTOCOLS_DIR}/unstable/wlr-layer-shell-unstable-v1.xml")
set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")

# Generate Protocols
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h" "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.c"
    COMMAND ${WAYLAND_SCANNER} client-header ${LAYER_SHELL_XML} "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h"
    COMMAND ${WAYLAND_SCANNER} private-code ${LAYER_SHELL_XML} "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.c"
    DEPENDS ${LAYER_SHELL_XML}
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h" "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
    DEPENDS ${XDG_SHELL_XML}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${WAYLAND_CLIENT_INCLUDE_DIRS} ${STB_INCLUDE_DIRS} src)

set(SOURCES
    src/main.cpp
    src/common.cpp
    src/config.cpp
    src/ipc.cpp
    src/renderer.cpp
    src/wayland_backend.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.c"
    "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
)

add_executable(waul ${SOURCES})

# Apply C++ specific flags ONLY to the target
target_compile_options(waul PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Os -fno-exceptions -fno-rtti -Wall -Wextra>)
target_compile_options(waul PRIVATE $<$<COMPILE_LANGUAGE:C>:-Os -Wall -Wextra>)

target_link_libraries(waul ${WAYLAND_CLIENT_LIBRARIES} pthread)
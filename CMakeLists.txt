cmake_minimum_required(VERSION 3.20)
project(wlcomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots-0.20)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED IMPORTED_TARGET wayland-protocols)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)

pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_HEADER "${CMAKE_BINARY_DIR}/protocols/xdg-shell-protocol-h")

add_custom_command(
  OUTPUT ${XDG_SHELL_HEADER}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/protocols
  COMMAND ${WAYLAND_SCANNER} server-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
  DEPENDS ${XDG_SHELL_XML})
add_custom_target(generate-protocols DEPENDS ${XDG_SHELL_HEADER})

add_executable(wlcomp src/main.cpp)
add_dependencies(wlcomp generate-protocols)

target_include_directories(wlcomp PRIVATE ${CMAKE_BINARY_DIR}/protocols
	${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(wlcomp PRIVATE WLR_USE_UNSTABLE)
target_link_libraries(
  wlcomp PRIVATE PkgConfig::WLROOTS PkgConfig::WAYLAND_SERVER
                 PkgConfig::XKBCOMMON PkgConfig::PIXMAN)

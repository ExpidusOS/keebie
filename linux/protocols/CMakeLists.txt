pkg_get_variable(WAYLAND_SCANNER_BINDIR wayland-scanner bindir)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

set(WAYLAND_SCANNER ${WAYLAND_SCANNER_BINDIR}/wayland-scanner)
set(WAYLAND_PROTOCOLS "${CMAKE_CURRENT_SOURCE_DIR}/input-method-unstable-v2.xml" "${CMAKE_CURRENT_SOURCE_DIR}/virtual-keyboard-unstable-v1.xml")
set(WAYLAND_PROTOCOLS_GEN)

foreach(PROTO IN ITEMS ${WAYLAND_PROTOCOLS})
  get_filename_component(PROTO_NAME "${PROTO}" NAME_WE)

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}-client.h"
    DEPENDS ${PROTO}
    COMMAND ${WAYLAND_SCANNER} client-header ${PROTO} ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}-client.h)

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}-public.c"
    DEPENDS ${PROTO} "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}-client.h"
    COMMAND ${WAYLAND_SCANNER} public-code ${PROTO} ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}-public.c)

  list(APPEND WAYLAND_PROTOCOLS_GEN "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}-public.c")
endforeach()

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client)
add_library(wayland-protocols STATIC ${WAYLAND_PROTOCOLS_GEN})
target_link_libraries(wayland-protocols PRIVATE PkgConfig::WAYLAND)
target_include_directories(wayland-protocols INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
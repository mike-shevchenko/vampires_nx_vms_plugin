# Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

set(vampires_nx_vms_plugin_src_dir ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE vampires_nx_vms_plugin_src CONFIGURE_DEPENDS
    ${vampires_nx_vms_plugin_src_dir}/*)

add_library(vampires_nx_vms_plugin SHARED ${vampires_nx_vms_plugin_src})
target_include_directories(vampires_nx_vms_plugin PRIVATE ${vampires_nx_vms_plugin_src_dir})
target_link_libraries(vampires_nx_vms_plugin PRIVATE nx_kit nx_sdk ws2_32)

target_compile_definitions(vampires_nx_vms_plugin PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO})

add_plugin_to_analytics_plugin_ut_cfg(vampires_nx_vms_plugin ${CMAKE_CURRENT_BINARY_DIR})

## Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

add_executable(nx_sdk_ut
    src/ref_countable_ut.cpp
    src/ptr_ut.cpp
    src/uuid_helper_ut.cpp
    src/main.cpp
)

target_link_libraries(nx_sdk_ut
    nx_kit
    nx_sdk
)

target_compile_definitions(nx_sdk_ut PRIVATE NX_PLUGIN_API=)

add_test(NAME nx_sdk_ut COMMAND nx_sdk_ut)
set_target_properties(nx_sdk_ut PROPERTIES FOLDER sdk)

#--------------------------------------------------------------------------------------------------
# Define analytics_plugin_ut - a unit test which tests an arbitrary list of Analytics Plugins.
#
# The list of plugin dynamic libraries must be specified in analytics_plugin_ut.cfg located next
# to the unit test executable.

add_executable(analytics_plugin_ut
    src/analytics_plugin_ut.cpp
)

target_link_libraries(analytics_plugin_ut PRIVATE nx_kit nx_sdk)
if(NOT WIN32)
    target_link_libraries(analytics_plugin_ut
        PRIVATE
            dl
            # The libpthread library must be explicitly linked, because it is used by libstdc++ via
            # weak symbols instead of being listed as a dependency. The option -pthread (which
            # implies -lpthread) is not enough here, because in some linkers the option --as-needed
            # is too agressive and optimizes away libpthread.
            -Wl,--no-as-needed pthread -Wl,--as-needed
    )
    # The option -pthread is needed in addition to linking with libpthread.
    set_target_properties(analytics_plugin_ut PROPERTIES LINK_FLAGS -pthread)
endif()

target_compile_definitions(analytics_plugin_ut PRIVATE NX_SDK_API=) #< for nxLibContext()

add_test(NAME analytics_plugin_ut COMMAND analytics_plugin_ut) #< Register the executable in ctest.

# Needed for add_plugin_to_analytics_plugin_ut_cfg().
set_property(GLOBAL PROPERTY analytics_plugin_ut_binary_dir ${CMAKE_CURRENT_BINARY_DIR})

# Generates analytics_plugin_ut.cfg with the specified list of plugin libraries.
function(add_plugin_to_analytics_plugin_ut_cfg plugin_target plugin_binary_dir)
    get_property(analytics_plugin_ut_binary_dir GLOBAL PROPERTY analytics_plugin_ut_binary_dir)
    set(testConfigFile ${analytics_plugin_ut_binary_dir}/analytics_plugin_ut.cfg)
    
    if(NOT EXISTS ${testConfigFile})
        file(WRITE ${testConfigFile} "# Paths to plugin library files which must be tested.\n")
    endif()

    if(WIN32)
        set(libPrefix "")
        set(libSuffix ".dll")
    else()
        set(libPrefix "lib")
        set(libSuffix ".so")
    endif()

    file(RELATIVE_PATH plugin_binary_dir_relative
        ${analytics_plugin_ut_binary_dir} ${plugin_binary_dir})

    set(plugin_library_filename ${libPrefix}${plugin_target}${libSuffix})

    file(APPEND ${testConfigFile} "${plugin_binary_dir_relative}/${plugin_library_filename}\n")
endfunction()

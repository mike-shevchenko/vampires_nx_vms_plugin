## Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${SRC_DIR}/*)

add_executable(ms_netcat ${SRC})

if(WIN32)
    set_target_properties(ms_netcat PROPERTIES WIN32_EXECUTABLE OFF) #< Build a console app.
endif()

target_link_libraries(ms_netcat PRIVATE nx_kit ws2_32)

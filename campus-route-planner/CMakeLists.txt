cmake_minimum_required(VERSION 3.20)
project(CampusRoutePlanner VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(STRICT_WARNINGS "Treat compiler warnings as errors" OFF)

add_library(route_core src/graph.c src/pathfinder.c src/input.c)
target_include_directories(route_core PUBLIC include)
add_executable(campus_route_planner src/main.c)
target_link_libraries(campus_route_planner PRIVATE route_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(route_tests tests/test_runner.c)
    target_link_libraries(route_tests PRIVATE route_core)
    add_test(NAME core_tests COMMAND route_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    set_tests_properties(core_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME cli_tests COMMAND ${CMAKE_COMMAND}
        "-DAPP=$<TARGET_FILE:campus_route_planner>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/cli-fixtures"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake")
endif()

foreach(target route_core campus_route_planner route_tests)
    if(TARGET ${target})
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /utf-8)
            target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
            if(STRICT_WARNINGS)
                target_compile_options(${target} PRIVATE /WX)
            endif()
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
            if(STRICT_WARNINGS)
                target_compile_options(${target} PRIVATE -Werror)
            endif()
        endif()
    endif()
endforeach()

add_custom_command(TARGET campus_route_planner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:campus_route_planner>/data")
set_target_properties(campus_route_planner PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:campus_route_planner>")

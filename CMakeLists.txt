cmake_minimum_required(VERSION 3.16)
project(CampusTreeManager VERSION 1.0 LANGUAGES C)

add_executable(CampusTreeManager src/main.c src/tree_manager.c)
set_target_properties(CampusTreeManager PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
if(MSVC)
    target_compile_options(CampusTreeManager PRIVATE /W4 /utf-8)
    target_compile_definitions(CampusTreeManager PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(CampusTreeManager PRIVATE -Wall -Wextra -Wpedantic)
endif()

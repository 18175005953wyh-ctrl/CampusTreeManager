cmake_minimum_required(VERSION 3.16)
project(CampusTreeManager VERSION 2.0 LANGUAGES C)

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

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME campus_tree_integration
            COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
                    $<TARGET_FILE:CampusTreeManager>)
        set_tests_properties(campus_tree_integration PROPERTIES TIMEOUT 180)
    else()
        message(WARNING "Python3 not found: integration checks are not registered.")
    endif()
    add_executable(tree_save_fault_tests tests/test_save_failures.c)
    set_target_properties(tree_save_fault_tests PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    if(MSVC)
        target_compile_options(tree_save_fault_tests PRIVATE /W4 /utf-8)
        target_compile_definitions(tree_save_fault_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(tree_save_fault_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/save-fault-fixture")
    add_test(NAME campus_tree_save_failures COMMAND tree_save_fault_tests)
    set_tests_properties(campus_tree_save_failures PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/save-fault-fixture" TIMEOUT 30)
endif()

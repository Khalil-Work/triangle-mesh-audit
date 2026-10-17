cmake_minimum_required(VERSION 3.16)
project(triangle_mesh_audit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(stl_audit STATIC src/stl_audit.cpp)
target_include_directories(stl_audit PUBLIC src)
if(MSVC)
    target_compile_options(stl_audit PRIVATE /W4)
else()
    target_compile_options(stl_audit PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(test_stl_audit tests/test_stl_audit.cpp)
target_link_libraries(test_stl_audit PRIVATE stl_audit)
if(MSVC)
    target_compile_options(test_stl_audit PRIVATE /W4)
else()
    target_compile_options(test_stl_audit PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME stl_audit_checks COMMAND test_stl_audit
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(stl_demo src/stl_demo.cpp)
target_link_libraries(stl_demo PRIVATE stl_audit)
if(MSVC)
    target_compile_options(stl_demo PRIVATE /W4)
else()
    target_compile_options(stl_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()

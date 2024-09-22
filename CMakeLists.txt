cmake_minimum_required(VERSION 3.18)

project(github_hosts 
    VERSION 1.0
    DESCRIPTION "main project"
    HOMEPAGE_URL "https://github.com/1995zyl/github_hosts"
    LANGUAGES CXX
)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
link_directories(${LIBRARY_OUTPUT_PATH})

add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)
add_subdirectory(src)

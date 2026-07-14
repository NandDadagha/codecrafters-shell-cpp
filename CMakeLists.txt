cmake_minimum_required(VERSION 3.13)

project(codecrafters-shell)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(shell ${SOURCE_FILES})

target_include_directories(shell PRIVATE include)

target_link_libraries(shell PRIVATE readline)
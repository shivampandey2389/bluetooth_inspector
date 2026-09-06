cmake_minimum_required(VERSION 3.16)

project(bluetooth-inspector
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Symlink compile_commands.json to source root for IDEs and language servers
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/compile_commands.json")
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
        SYMBOLIC
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

add_executable(bluetooth-inspector
    src/main.cpp
    src/bluez_types.h
    src/bluez_object_manager.h
    src/bluez_object_manager.cpp
)

target_include_directories(bluetooth-inspector
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(bluetooth-inspector
    PRIVATE
        Qt6::Core
        Qt6::DBus
)

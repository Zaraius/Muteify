cmake_minimum_required(VERSION 3.10)
project(Muteify C)

find_package(CURL REQUIRED)
find_package(cJSON REQUIRED)

add_executable(Muteify
    main.c
    get.c
)

configure_file(${CMAKE_SOURCE_DIR}/token.txt ${CMAKE_BINARY_DIR}/token.txt COPYONLY)

target_link_libraries(Muteify PRIVATE CURL::libcurl cjson)
target_include_directories(Muteify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

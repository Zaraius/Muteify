# Find required packages
find_package(CURL REQUIRED)
find_package(cJSON REQUIRED)

# Create library for Spotify API functionality (has both .c and .h)
add_library(spotify_api
    get.c
    get.h
)

# Link external libraries to the spotify_api library
# PUBLIC because get.h exposes functions that use these libraries
target_link_libraries(spotify_api PUBLIC CURL::libcurl cjson)

# Make headers accessible to other targets
target_include_directories(spotify_api PUBLIC ${CMAKE_SOURCE_DIR})

# Create executable for main program
add_executable(muteify
    main.c
)

# Link the spotify_api library to the executable
# PRIVATE because main.c uses the library internally
target_link_libraries(muteify PRIVATE spotify_api)

# Copy token file to build directory
configure_file(${CMAKE_SOURCE_DIR}/token.txt ${CMAKE_BINARY_DIR}/token.txt COPYONLY)
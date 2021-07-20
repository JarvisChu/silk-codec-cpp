cmake_minimum_required(VERSION 3.6)
project(silk)

include_directories(
    interface
    src
)

aux_source_directory(src silk_SOURCES)
#aux_source_directory(interface silk_public_HEADERS)

#set(SRCS ${silk_SOURCES} ${silk_public_HEADERS})

if (NOT WIN32 AND NOT APPLE)
    add_compile_options(-Wall -fPIC)
endif()

if (NOT WIN32)
    add_compile_options(-Wno-shift-negative-value)
endif()

add_library(${PROJECT_NAME} STATIC ${silk_SOURCES})

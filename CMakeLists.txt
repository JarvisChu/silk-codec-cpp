cmake_minimum_required(VERSION 3.5.1)

project(SilkCodecDemo C CXX)

set(CMAKE_CXX_STANDARD 11)

include_directories(
    .
    silk/interface
    silk/src
)

add_subdirectory(silk)

aux_source_directory(. CODEC_SRCS)
set(SRCS ${CODEC_SRCS})

add_executable(SilkCodecDemo ${SRCS})
target_link_libraries(SilkCodecDemo silk)


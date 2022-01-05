cmake_minimum_required(VERSION 3.0.0)
project(fanju VERSION 0.1.0)


add_library(fanju SHARED fanju.cpp ELECHOUSE_CC1101_SRC_DRV.cpp)

target_link_libraries(fanju pigpiod_if2)

target_include_directories(fanju PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}"
                          )

add_executable(fanjucli main.cpp)
target_link_libraries(fanjucli fanju)
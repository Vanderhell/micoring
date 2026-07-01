cmake_minimum_required(VERSION 3.21)
project(micoring_install_consumer LANGUAGES C)

find_package(micoring CONFIG REQUIRED)

add_executable(micoring_install_consumer main.c)
target_compile_features(micoring_install_consumer PRIVATE c_std_99)
target_link_libraries(micoring_install_consumer PRIVATE micoring::micoring)

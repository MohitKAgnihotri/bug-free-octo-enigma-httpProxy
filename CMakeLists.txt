cmake_minimum_required(VERSION 3.16)
project(bug_free_octo_enigma_httpProxy C)

set(CMAKE_C_STANDARD 11)
link_libraries(pthread)

add_executable(bug_free_octo_enigma_httpProxy server.c server.h)

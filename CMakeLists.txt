cmake_minimum_required(VERSION 3.16)
project(bug_free_octo_enigma_httpProxy C)

set(CMAKE_C_STANDARD 11)
link_libraries(pthread)
set(CC_MAKE_FLAGS "-std=c11 -g3 -O0 -Wall -Wextra -Werror -Wpedantic")

add_executable(bug_free_octo_enigma_httpProxy server.c server.h)

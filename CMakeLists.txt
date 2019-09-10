cmake_minimum_required(VERSION 3.14)
project(2310hub C)

set(CMAKE_C_STANDARD 99)

add_executable(2310hub 2310hub.c parse.c 2310alice.c 2310bob.c)
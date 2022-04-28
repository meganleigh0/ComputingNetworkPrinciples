cmake_minimum_required(VERSION 3.9)
project(P4Server C)

set(CMAKE_C_STANDARD 99)

add_executable(P4Server server.c server.h GameLogic.c GameLogic.h)

cmake_minimum_required(VERSION 3.16)
project(client_naas C)

set(CMAKE_C_STANDARD 11)

add_executable(client_naas src/main.c src/tun.c include/tun.h src/daemonize.c include/daemonize.h include/msg.h src/msg.c src/log.c include/log.h)
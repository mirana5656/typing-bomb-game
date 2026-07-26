cmake_minimum_required(VERSION 4.1)
project(TypingoftheBombs C)

set(CMAKE_C_STANDARD 11)

add_executable(TypingoftheBombs main.c)

file(COPY plane.txt bomb.txt city.txt codewords.txt
     DESTINATION ${CMAKE_BINARY_DIR})

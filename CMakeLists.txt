cmake_minimum_required(VERSION 4.1)
project(ft_malloc C)

set(CMAKE_C_STANDARD 11)

add_library(ft_malloc SHARED library.c)

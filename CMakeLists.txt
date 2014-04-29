cmake_minimum_required(VERSION 2.8.9)

project(toxsh)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_FLAGS "-g -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")  #-std=c++11 -stdlib=libc++ -lc++abi
# add_definitions(-g -O2)

set(CMAKE_INCLUDE_CURRENT_DIR on)
set(CMAKE_AUTOMOC on)

find_package(Qt5Core)

# TODO use pkg_check_module
include_directories("/serv/stow/ptox/include" "/serv/stow/ptox/include/tox"
  "./common")
link_directories("/serv/stow/ptox/lib")
#link_libraries(-ltoxcore)


add_subdirectory(toxshd)
add_subdirectory(toxshc)
add_subdirectory(webshd)
add_subdirectory(webshc)
add_subdirectory(trohd)
add_subdirectory(trohc)
add_subdirectory(dtnat)
add_subdirectory(nathlp)


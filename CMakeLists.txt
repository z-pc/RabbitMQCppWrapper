cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(wrapper_ex C CXX)

find_package(rabbitmq-c CONFIG REQUIRED)

add_executable(wrapper_ex 
     example.cpp 
     src/noncopyable.h
     src/Connection.h
     src/Connection.cpp
     src/Channel.h
     src/Channel.cpp
     src/Exception.h
     src/Exception.cpp)

target_link_libraries(wrapper_ex PUBLIC rabbitmq::rabbitmq)

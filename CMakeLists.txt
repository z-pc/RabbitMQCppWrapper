cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(wrapper_ex C CXX)

find_package(rabbitmq-c CONFIG REQUIRED)

set(WRAPPER_SRC 
     src/noncopyable.h
     src/Connection.h
     src/Connection.cpp
     src/Channel.h
     src/Channel.cpp
     src/Exception.h
     src/Exception.cpp)

list(APPEND WRAPPER_EXAMPLES
ex_fully 
ex_consumer
ex_publisher
ex_queue
ex_exchange
)

foreach(example IN LISTS WRAPPER_EXAMPLES)
     add_executable(${example} examples/${example}.cpp ${WRAPPER_SRC})
     target_link_libraries(${example} PUBLIC rabbitmq::rabbitmq )
endforeach()
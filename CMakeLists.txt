cmake_minimum_required(VERSION 3.15)
project(mpi)
set(CMAKE_CXX_COMPILER "mpic++")

set(Boost_USE_MULTITHREADED TRUE)
find_package(Boost COMPONENTS mpi serialization REQUIRED)
find_package(PNG REQUIRED)

if(Boost_FOUND)
    message("!!! Boost found !!!")
    include_directories(${Boost_INCLUDE_DIRS})
endif()
if (PNG_FOUND)
    message("!!! PNG found !!!")
    include_directories(${PNG_INCLUDE_DIR})
endif()

add_executable(mpi main.cpp other.h)
target_link_libraries(mpi ${Boost_LIBRARIES} ${PNG_LIBRARY})



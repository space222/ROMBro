cmake_minimum_required(VERSION 3.10)

# set the project name
project(rombro)

find_package(SDL2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O2")

# add the executable
add_executable(rombro main.cpp system.cpp gfx.cpp gbcpu.cpp io.cpp memory.cpp mbc.cpp sound.cpp)

target_include_directories(rombro PUBLIC ${SDL2_INCLUDE_DIRS} )

target_link_libraries(rombro PUBLIC dl GL GLEW ${SDL2_LIBRARIES})




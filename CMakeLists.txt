set(CMAKE_CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.10)
project(snd)
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    winmm.lib
)
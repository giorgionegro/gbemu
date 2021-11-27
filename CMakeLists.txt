cmake_minimum_required(VERSION 3.0)
project(untitled)
#[[add gnu c extension]]
#[[add gnu c++ extension]]
#sdl2
#[[add sdl2 extension]]
#[[add sdl2_image extension]]
#[[add sdl2_ttf extension]]
#[[add sdl2_mixer extension]]
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -lmingw32 -mbmi2 -O3")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -pthread -lSDL2main -lSDL2 -lSDL2_ttf -O3")



set(SOURCE_FILES main.cpp)
set(CMAKE_CXX_STANDARD 23)
add_executable(untitled main.cpp)
target_link_libraries(untitled mingw32 SDL2main SDL2 SDL2_ttf)






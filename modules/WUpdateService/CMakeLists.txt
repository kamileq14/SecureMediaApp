cmake_minimum_required(VERSION 3.25)
project(WUpdateService)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    # Ustawienia dla aplikacji Windows GUI
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

add_executable(WUpdateService main.cpp)
target_link_libraries(WUpdateService ws2_32)

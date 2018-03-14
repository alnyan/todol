project(todol)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

option(WITH_AT "Include at/libnotify notification support" ON)
option(WITH_WEB "Include simple task web-server" OFF)
option(WITH_COLOR "Include terminal color support (xterm)" ON)

set(SOURCES src/main.cpp src/todol.cpp src/todol.hpp src/todol_storage.cpp src/todol_storage.hpp)
set(LIBRARIES "")

if (${WITH_COLOR})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_COLOR")
    set(SOURCES ${SOURCES} src/color.hpp)
endif ()

if (${WITH_WEB})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_WEB")
    set(SOURCES ${SOURCES} src/socket.cpp src/socket.hpp src/todol_web.cpp src/todol_web.hpp)
    set(LIBRARIES ${LIBRARIES} pthread)
endif ()

if (${WITH_AT})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_AT")
    set(SOURCES ${SOURCES} src/timeutil.cpp src/timeutil.hpp src/todol_at.cpp src/todol_at.hpp)
endif ()

add_executable(todol ${SOURCES})
target_link_libraries(todol ${LIBRARIES})

install(TARGETS todol DESTINATION bin)

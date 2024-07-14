
cmake_minimum_required(VERSION 3.7...3.15)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif ()
project(canbus LANGUAGES C CXX)

include_directories(lib)
include_directories(src)

set(SRC lib/lib_iqueue.c src/lib_iso15765.c src/lib_iso15765_clear.c
    src/lib_utils.c
    )


add_executable(canbus_exe ${SRC} exm/ISO15765Development.c)
add_compile_definitions(canbus_exe ISO16765Example)

add_executable(queue_demo_exe ${SRC} queue_demo/queue_demo.c)

link_libraries(pthread)
add_executable(can_sender ${SRC} src/cansocket.c can_sender/can_sender.c)
add_executable(can_receiver ${SRC} src/cansocket.c can_receiver/can_receiver.c)

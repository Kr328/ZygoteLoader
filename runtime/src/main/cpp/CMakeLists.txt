cmake_minimum_required(VERSION 3.10.2)

project(ZygoteLoader)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(C_FLAGS "${C_FLAGS} -O2 -fvisibility=hidden -fvisibility-inlines-hidden")
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-exclude-libs,ALL -Wl,--gc-sections -flto")
else ()
    add_definitions(-DDEBUG)
    set(C_FLAGS "${C_FLAGS} -O0")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

set(MODULE_BASE_SOURCES serializer.c properties.c process.c linker.c dex.c main.c mem.cpp)

if(LOADER STREQUAL "zygisk")
    add_library(zygisk_loader SHARED main_zygisk.cpp ${MODULE_BASE_SOURCES})
    target_link_libraries(zygisk_loader log)

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET zygisk_loader POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libzygisk_loader.so")
    endif()
elseif(LOADER STREQUAL "riru")
    find_package(riru REQUIRED CONFIG)

    add_library(riru_loader SHARED main_riru.cpp ${MODULE_BASE_SOURCES})
    target_link_libraries(riru_loader log riru::riru)

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET riru_loader POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libriru_loader.so")
    endif()
else()
    message(FATAL_ERROR "Unknown loader: ${LOADER}")
endif()

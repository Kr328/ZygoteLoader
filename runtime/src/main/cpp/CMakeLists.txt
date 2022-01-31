cmake_minimum_required(VERSION 3.10.2)

project(ZygoteLoader CXX)

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)

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

find_package(cxx REQUIRED CONFIG)

add_library(base STATIC entrypoint.cpp chunk.cpp properties.cpp scoped.cpp dex.cpp logger.cpp utils.cpp)
target_link_libraries(base log cxx::cxx)

if(LOADER STREQUAL "zygisk")
    add_library(zygisk_loader SHARED delegate_zygisk.cpp)
    target_link_libraries(zygisk_loader base)

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET zygisk_loader POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libzygisk_loader.so")
    endif()
elseif(LOADER STREQUAL "riru")
    find_package(riru REQUIRED CONFIG)

    add_library(riru_loader SHARED delegate_riru.cpp)
    target_link_libraries(riru_loader base riru::riru)

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET riru_loader POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libriru_loader.so")
    endif()
else()
    message(FATAL_ERROR "Unknown loader: ${LOADER}")
endif()

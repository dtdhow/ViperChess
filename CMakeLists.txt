cmake_minimum_required(VERSION 3.15)
project(ViperChess-Mega)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -march=native")

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
    
    # MinGW-specific ZLIB paths
    set(ZLIB_NAMES z zlib1 zlib zdll zlibstatic)
    find_library(ZLIB_LIBRARY
        NAMES ${ZLIB_NAMES}
        PATHS 
            ${CMAKE_SOURCE_DIR}/libs
            ${CMAKE_SOURCE_DIR}/lib
            "C:/MinGW/lib"
            "C:/mingw64/lib"
        PATH_SUFFIXES lib lib64
        REQUIRED
    )
    
    find_path(ZLIB_INCLUDE_DIR
        NAMES zlib.h
        PATHS 
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/zlib/include
            "C:/MinGW/include"
            "C:/mingw64/include"
        REQUIRED
    )
    
    if(ZLIB_LIBRARY AND ZLIB_INCLUDE_DIR)
        message(STATUS "Found ZLIB: ${ZLIB_LIBRARY}")
        add_library(ZLIB::ZLIB STATIC IMPORTED)
        set_target_properties(ZLIB::ZLIB PROPERTIES
            IMPORTED_LOCATION "${ZLIB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}"
        )
    else()
        message(FATAL_ERROR "ZLIB not found. Please install with: pacman -S mingw-w64-x86_64-zlib")
    endif()
endif()

# Fathom setup
set(FATHOM_DIR ${CMAKE_SOURCE_DIR}/Fathom)
if(EXISTS "${FATHOM_DIR}/libfathom.a")
    add_library(fathom STATIC IMPORTED)
    set_target_properties(fathom PROPERTIES
        IMPORTED_LOCATION "${FATHOM_DIR}/libfathom.a"
        INTERFACE_INCLUDE_DIRECTORIES "${FATHOM_DIR}"
    )
else()
    message(WARNING "Fathom library not found at ${FATHOM_DIR}/libfathom.a")
endif()

add_executable(viperchess
    src/main.cpp
    src/board.cpp
    src/eval.cpp
    src/search.cpp
    src/uci.cpp
    src/book.cpp
)

target_include_directories(viperchess PRIVATE 
    include
    ${ZLIB_INCLUDE_DIR}
    ${FATHOM_DIR}
)

target_link_libraries(viperchess PRIVATE
    ZLIB::ZLIB
)

if(TARGET fathom)
    target_link_libraries(viperchess PRIVATE fathom)
endif()

# Windows console subsystem
if(WIN32)
    set_target_properties(viperchess PROPERTIES
        LINK_FLAGS "-Wl,-subsystem,console"
    )
endif()
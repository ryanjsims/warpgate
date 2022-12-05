project(lzham)
cmake_minimum_required(VERSION 3.12)

if(MSVC)
    set(threading_src ../lzham_codec/lzhamcomp/lzham_win32_threading.cpp)
else()
    set(threading_src ../lzham_codec/lzhamcomp/lzham_pthreads_threading.cpp)
endif()

add_library(lzham STATIC 
    ../lzham_alpha/lzhamdecomp/lzham_assert.cpp
    ../lzham_alpha/lzhamdecomp/lzham_checksum.cpp
    ../lzham_alpha/lzhamdecomp/lzham_huffman_codes.cpp
    ../lzham_alpha/lzhamdecomp/lzham_lzdecompbase.cpp
    ../lzham_alpha/lzhamdecomp/lzham_lzdecomp.cpp
    ../lzham_alpha/lzhamdecomp/lzham_mem.cpp
    ../lzham_alpha/lzhamdecomp/lzham_platform.cpp
    ../lzham_alpha/lzhamdecomp/lzham_polar_codes.cpp
    ../lzham_alpha/lzhamdecomp/lzham_prefix_coding.cpp
    ../lzham_alpha/lzhamdecomp/lzham_symbol_codec.cpp
    ../lzham_alpha/lzhamdecomp/lzham_timer.cpp
    ../lzham_alpha/lzhamdecomp/lzham_vector.cpp
    ../lzham_alpha/lzhamcomp/lzham_lzbase.cpp
    ../lzham_alpha/lzhamcomp/lzham_lzcomp.cpp
    ../lzham_alpha/lzhamcomp/lzham_lzcomp_internal.cpp
    ../lzham_alpha/lzhamcomp/lzham_lzcomp_state.cpp
    ../lzham_alpha/lzhamcomp/lzham_match_accel.cpp
    ../lzham_alpha/lzhamcomp/lzham_lzcomp_internal.cpp
    ${threading_src}
    ../lzham_alpha/lzhamlib/lzham_lib.cpp
)
target_include_directories(lzham 
    PUBLIC ../lzham_alpha/include ../lzham_alpha/lzhamdecomp ../lzham_alpha/lzhamcomp
)

if(NOT MSVC)
    target_compile_options(lzham PRIVATE "-Wno-unused-value")
endif()


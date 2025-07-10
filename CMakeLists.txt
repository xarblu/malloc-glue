cmake_minimum_required(VERSION 3.10)
project(
    mimalloc-glue
    VERSION 0.1.0
    LANGUAGES C
)

# apparently we're using clang extensions
set(CMAKE_C_COMPILER clang)

# libmimalloc-glue.so
add_library(mimalloc-glue SHARED mimalloc-glue.c)
target_sources(mimalloc-glue PRIVATE mimalloc-glue.c)
target_compile_options(mimalloc-glue PRIVATE -Wall -Wextra -Werror)

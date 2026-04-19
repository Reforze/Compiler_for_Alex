cmake_minimum_required(VERSION 3.10)
project(AlexLang CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(alexc
    main.cpp
    Lexer/lexer.cpp
    Parser/parser.cpp
    Semantic/semantic.cpp
    CodeGen/codegen.cpp
)

target_include_directories(alexc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

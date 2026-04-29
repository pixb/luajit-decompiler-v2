cmake_minimum_required(VERSION 3.16)
project(luajit-decompiler-v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/J)
else()
    add_compile_options(-funsigned-char)
endif()

add_executable(luajit-decompiler-v2
    main.cpp
    bytecode/bytecode.cpp
    bytecode/prototype.cpp
    ast/ast.cpp
    lua/lua.cpp
)

target_include_directories(luajit-decompiler-v2 PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/bytecode
    ${CMAKE_SOURCE_DIR}/ast
    ${CMAKE_SOURCE_DIR}/lua
)

if(WIN32)
    target_link_libraries(luajit-decompiler-v2 PRIVATE shlwapi)
endif()
cmake_minimum_required(VERSION 3.29)
project(bookstore)

set(CMAKE_CXX_STANDARD 20)

add_executable(bs
        src/main.cpp
        src/ci.cpp
        src/ci.h
        src/error.h
        src/fs.cpp
        src/fs.h
        src/database.cpp
        src/database.h
        src/database_more.h
        src/account.cpp
        src/account.h
        src/util.h)
add_executable(bookstore-pre dist/bookstore-pre.cpp)

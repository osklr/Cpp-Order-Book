cmake_minimum_required(VERSION 3.15)
project(CPP-ORDER-BOOK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(order_book main.cpp order_book.cpp trade_journal.cpp order_journal.cpp)

enable_testing()

find_package(GTest REQUIRED)

add_executable(order_book_test tests/tests.cpp order_book.cpp trade_journal.cpp order_journal.cpp)

target_include_directories(order_book_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(order_book_test GTest::gtest_main)
include(GoogleTest)

gtest_discover_tests(order_book_test)
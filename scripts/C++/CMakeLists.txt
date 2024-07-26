cmake_minimum_required(VERSION 3.22)
project(cpp_portfolio_modelling)

set(CMAKE_CXX_STANDARD 20)

include_directories(yfinance)
include_directories(nlohmann)

add_executable(cpp_portfolio_modelling main.cpp Miner.cpp Miner.h Portfolio.cpp Portfolio.h)

target_link_libraries(cpp_portfolio_modelling "${CMAKE_SOURCE_DIR}/libyahoo-finance.a" curl)

find_package(Threads)
target_link_libraries(cpp_portfolio_modelling ${CMAKE_THREAD_LIBS_INIT})
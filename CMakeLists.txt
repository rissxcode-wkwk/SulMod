cmake_minimum_required(VERSION 3.21)
project(SuLMod VERSION 1.0.0)

# Tambahkan semua file source
file(GLOB SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Ambil Geode SDK (pastikan environment variable GEODE_SDK sudah diset,
# atau clone geode-sdk/geode lalu arahkan ke foldernya)
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Variabel environment GEODE_SDK belum diset! Install Geode CLI dulu ya (geode-sdk.org/docs)")
else()
    message(STATUS "Menemukan Geode SDK: $ENV{GEODE_SDK}")
    add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
endif()

setup_geode_mod(${PROJECT_NAME})

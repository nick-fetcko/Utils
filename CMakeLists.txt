cmake_minimum_required(VERSION 3.9.0 FATAL_ERROR)
project(Utils VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

set(_utils_headers
	Base64.hpp
	Hash.hpp
	Utils.hpp
	Logger.hpp
	ShiftJIS.hpp
	Windows1252.hpp
	)
set(_utils_sources
	Utils.cpp
	Logger.cpp
	ShiftJIS.cpp
	)

add_library(Utils STATIC ${_utils_headers} ${_utils_sources})
set_target_properties(Utils PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(Utils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_features(Utils PUBLIC cxx_std_17)
target_compile_definitions(Utils PUBLIC _CRT_SECURE_NO_WARNINGS)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Here lies wstring_convert
	# C++11 - C++17
	# Gone Too Soon
	target_compile_options(Utils PRIVATE -Wno-deprecated-declarations)
endif()
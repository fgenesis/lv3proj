cmake_minimum_required(VERSION 2.6)

# if no build type was provided, set a default one (taken from sfml2 cmake file)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

PROJECT(LV3)
add_subdirectory (src) 

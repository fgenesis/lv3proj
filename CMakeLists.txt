cmake_minimum_required(VERSION 2.6)
PROJECT(LV3)

# TODO: - make it build on macosx

message(STATUS "Using Generator: ${CMAKE_GENERATOR}")

if(MSVC)
    message(ERROR "Use the included MSVC solution to build!")
endif()

# fix up.
if(APPLE)
    set(MACOSX TRUE)
endif()

# HACK: in case mingw is not correctly detected, fix this.
if(${CMAKE_GENERATOR} MATCHES ".*[mM][iI][nN][gG][wW].*") # really ugly hack to match "mingw" case-insensitive
    set(MINGW 1)
    message(STATUS "!! Using MinGW hack !!")
endif()

# setup some variables where to find what
set(TOP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/lib")
set(INCLUDE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/include")
set(BIN_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# platform specific (win32)
set(WIN32_LIB_DIR "${LIB_ROOT_DIR}/win32")

# platform specific (macosx) - TODO
set(MACOSX_LIB_DIR "${LIB_ROOT_DIR}/macosx") # not yet added


option(USE_INTERNAL_SDL "Always use included SDL library" FALSE)

# win32 does not have a package system, force using included libs
if(WIN32)
    set(USE_INTERNAL_SDL TRUE)
endif(WIN32)

# if no build type was provided, set a default one (taken from sfml2 cmake file)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Find SDL or use the included library if applicable
if(NOT USE_INTERNAL_SDL)
    find_package(SDL)
    find_package(SDL_mixer)
    find_package(SDL_image)
endif()
if(SDL_FOUND)
    message(STATUS "Found SDL: ${SDL_LIBRARY}")
    message(STATUS "Found SDL_image: ${SDL_image_LIBRARY}")
    message(STATUS "Found SDL_mixer: ${SDL_mixer_LIBRARY}")
else()
    if(NOT USE_INTERNAL_SDL)
        message("WARNING: Tried to find system SDL, but not found, falling back to included SDL")
    endif()
    if(MACOSX)
        message(STATUS "Using internal copy of SDL - DONT HAVE IT YET - FIXME")
        #set(SDL_LIBRARY "${MACOSX_LIB_DIR}/libSDL-1.2.0.dylib;${MACOSX_LIB_DIR}/libSDLmain.a")
    elseif(WIN32)
        message(STATUS "Using internal copy of SDL")
        if(MINGW)
            set(SDL_LIBRARY "mingw32 ${WIN32_LIB_DIR}/libSDLmain.a" "${WIN32_LIB_DIR}/libSDL.a" CACHE STRING "Where the SDL library can be found")
        else()
            set(SDL_LIBRARY "${WIN32_LIB_DIR}/SDLmain.lib" "${WIN32_LIB_DIR}/SDL.lib" CACHE STRING "Where the SDL library can be found")
        endif()
        set(SDL_image_LIBRARY "${WIN32_LIB_DIR}/SDL_image.lib" CACHE STRING "Where the SDL library can be found")
        set(SDL_mixer_LIBRARY "${WIN32_LIB_DIR}/SDL_mixer.lib" CACHE STRING "Where the SDL_mixer library can be found")
    else()
        message(ERROR "We don't have a prebuilt SDL for this platform.")
    endif()
    set(SDL_INCLUDE_DIR ${INCLUDE_ROOT_DIR}) 
    set(SDL_image_INCLUDE_DIR ${SDL_INCLUDE_DIR})
    set(SDL_mixer_INCLUDE_DIR ${SDL_INCLUDE_DIR})
endif ()

set(SDL_ALL_LIBS
    ${SDL_LIBRARY}
    ${SDL_image_LIBRARY}
    ${SDL_mixer_LIBRARY}
)

# extra libs that will be included after all others, by platform
if(MINGW)
    set(EXTRA_STANDARD_LIBS "-lwinmm")
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ${EXTRA_STANDARD_LIBS}")
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ${EXTRA_STANDARD_LIBS}")
endif()

message(STATUS "Libs: ${CMAKE_CXX_STANDARD_LIBRARIES}")

# default compiler flags for gcc, -O3 is known to break things, and we also need _DEBUG defined in debug mode
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR MINGW)
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used for release builds" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used for release builds" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "-ggdb -D_DEBUG" CACHE STRING "Flags used for debug builds" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -D_DEBUG" CACHE STRING "Flags used for debug builds" FORCE)
    add_definitions(-pipe -fsigned-char)
endif()

# install path
set(CMAKE_INSTALL_PREFIX "${TOP_ROOT_DIR}" CACHE STRING "Install directory (actual installs will go into 'bin' subdir of this). This is NOT a system install!" FORCE)


add_subdirectory (src) 

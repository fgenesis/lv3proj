
# top include dir for external projects
set(TOP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# shared include dir with most of the things required
set(SHARED_INCLUDE_DIR "${TOP_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/shared")

# framework
add_subdirectory (shared)
add_subdirectory (libpng)

# dependencies
add_subdirectory (falcon)
add_subdirectory (guichan)

# custom extensions
add_subdirectory (falcon_ext)
add_subdirectory (guichan_ext)

set(Falcon_ALL falcon_engine ${Falcon_MODULES})

# binaries
add_subdirectory (editor)
add_subdirectory (tilegrab)
add_subdirectory (lvpak)
add_subdirectory (game)


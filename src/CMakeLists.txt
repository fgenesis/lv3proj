
# top include dir for external projects
set(TOP_INCLUDE_DIR
"${CMAKE_CURRENT_SOURCE_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/include"
"${CMAKE_CURRENT_SOURCE_DIR}/guichan_ext"
)

# shared include dir with most of the things required
set(SHARED_INCLUDE_DIR "${TOP_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/shared")

# Falcon extra modules
set(Falcon_MODULES
  compiler_fm confparser_fm bufext_fm
)

set(Falcon_ALL ${Falcon_MODULES} falcon_engine)


# framework
add_subdirectory (shared)
add_subdirectory (libpng)

# dependencies
add_subdirectory (falcon)
add_subdirectory (guichan)

# custom extensions
add_subdirectory (falcon_ext)
add_subdirectory (guichan_ext)

# binaries
add_subdirectory (editor)
add_subdirectory (tilegrab)
add_subdirectory (lvpak)
add_subdirectory (game)


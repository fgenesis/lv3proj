
include_directories (${SHARED_INCLUDE_DIR})

add_executable (tilegrab 
main.cpp
) 

install(TARGETS tilegrab DESTINATION bin)
target_link_libraries(tilegrab shared libpng ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY})

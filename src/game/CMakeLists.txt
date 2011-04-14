
include_directories (${GAMEENGINE_INCLUDE_DIR}) 

add_executable (game 
main.cpp
game.rc
)


install(TARGETS game DESTINATION bin)
target_link_libraries(game gameengine shared guichan_ext guichan ${Falcon_ALL} ${SDL_ALL_LIBS})



include_directories (${SHARED_INCLUDE_DIR}) 

add_executable (game 
main.cpp
AppFalconGame.cpp
FalconGameModule.cpp
GameEngine.cpp
) 

install(TARGETS game DESTINATION bin)
target_link_libraries(game shared guichan_ext guichan ${Falcon_ALL} ${SDL_ALL_LIBS})

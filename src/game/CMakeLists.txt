
include_directories (${SHARED_INCLUDE_DIR}) 

add_executable (game 
main.cpp
AppFalconGame.cpp
FalconGameModule.cpp
GameEngine.cpp
) 

target_link_libraries(game shared ${Falcon_ALL} SDL SDL_mixer SDL_image)

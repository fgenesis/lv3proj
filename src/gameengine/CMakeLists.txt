
include_directories (${SHARED_INCLUDE_DIR}) 

add_library (gameengine 
AppFalconGame.cpp
FalconGameModule.cpp
GameEngine.cpp
) 

target_link_libraries(gameengine shared guichan_ext guichan ${Falcon_ALL} ${SDL_ALL_LIBS})

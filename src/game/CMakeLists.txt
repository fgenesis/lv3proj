include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/include) 
 include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/shared) 
# include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/guichan_ext) 
# 
# link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/shared) 
# link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/falcon) 
# link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/guichan) 

add_executable (game 
main.cpp
AppFalconGame.cpp
FalconGameModule.cpp
GameEngine.cpp
) 

target_link_libraries(game shared falcon_engine compiler_fm SDL SDL_mixer SDL_image)
 #shared falcon_engine compiler_fm SDL SDL_mixer SDL_image guichan   shared png SDL SDL_image
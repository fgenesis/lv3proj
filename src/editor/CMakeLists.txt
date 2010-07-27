include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/include) 
include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/shared) 
include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/guichan_ext) 

link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/shared) 
link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/falcon) 
link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/guichan) 

add_executable (editor EditorEngine.cpp
main.cpp
SDLImageLoaderManaged.cpp
SDLImageManaged.cpp
) 

target_link_libraries(editor shared falcon_engine compiler_fm SDL SDL_mixer SDL_image guichan )

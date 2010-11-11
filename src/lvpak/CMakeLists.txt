include_directories (${SHARED_INCLUDE_DIR}) 

add_executable (lvpak 
main.cpp
) 

target_link_libraries(lvpak shared)

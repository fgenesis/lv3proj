include_directories (${SHARED_INCLUDE_DIR}) 

add_executable (lvpak 
main.cpp
) 
install(TARGETS lvpak DESTINATION bin)
target_link_libraries(lvpak shared)

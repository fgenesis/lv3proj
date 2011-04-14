include_directories (${SHARED_INCLUDE_DIR}) 

add_executable (tests 
LVPACipherTests.cpp
LVPATests.cpp
main.cpp
) 
install(TARGETS tests DESTINATION bin)
target_link_libraries(tests shared)

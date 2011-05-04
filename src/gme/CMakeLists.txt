include_directories (${SHARED_INCLUDE_DIR}) 

file(GLOB gme_SRC *.c *.cpp)

add_library(gme ${gme_SRC})

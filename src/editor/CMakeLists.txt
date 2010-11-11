#include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/include)
#include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/shared)
#include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/guichan_ext)

#link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/shared)
#link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/falcon)
#link_directories (${LOSTVIKINGS3_BINARY_DIR}/src/guichan)


include_directories (${SHARED_INCLUDE_DIR})

set(editor_SRC
BottomBarPanel.cpp
DrawAreaPanel.cpp
EditorEngine.cpp
EventHandler.cpp
GUIHandler.cpp
LayerPanel.cpp
main.cpp
MapFileHandler.cpp
ObjectWindow.cpp
SavedDataHandler.cpp
TileboxPanel.cpp
TileLayerPanel.cpp
TilesHandler.cpp
TileWindow.cpp
)

add_executable (editor ${editor_SRC}) 

target_link_libraries(editor shared SDL SDL_mixer SDL_image guichan guichan_ext )

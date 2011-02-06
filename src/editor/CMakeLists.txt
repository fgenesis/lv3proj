
include_directories (${GAMEENGINE_INCLUDE_DIR})

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
install(TARGETS editor DESTINATION bin)
target_link_libraries(editor gameengine shared guichan_ext guichan ${Falcon_ALL} ${ALL_SDL_LIBS})

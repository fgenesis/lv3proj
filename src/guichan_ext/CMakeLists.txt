
include_directories (${SHARED_INCLUDE_DIR})

set(guichan_ext_SRC
FileDialog.cpp
GreedyTextField.cpp
ListBox2.cpp
MenuBar.cpp
MenuButton.cpp
Panel.cpp
PopupMenu.cpp
SDLImageLoaderManaged.cpp
SDLImageManaged.cpp
SelectionFramePanel.cpp
)

add_library(guichan_ext ${guichan_ext_SRC})

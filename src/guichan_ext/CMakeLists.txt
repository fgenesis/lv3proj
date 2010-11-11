
include_directories (${SHARED_INCLUDE_DIR})

set(guichan_ext_SRC
FileDialog.cpp
GreedyTextField.cpp
ListBox2.cpp
Panel.cpp
SDLImageLoaderManaged.cpp
SDLImageManaged.cpp
SelectionFramePanel.cpp
)

add_library(guichan_ext ${guichan_ext_SRC})

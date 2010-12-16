include_directories (${TOP_INCLUDE_DIR}) 

set(ALLEGRO
  allegro/allegroimage.cpp
  allegro/allegrofont.cpp
  allegro/allegrographics.cpp
  allegro/allegro.cpp
  allegro/allegroimageloader.cpp
  allegro/allegroinput.cpp
)
set(WIDGETS
widgets/listbox.cpp
widgets/textfield.cpp
widgets/imagebutton.cpp
widgets/dropdown.cpp
widgets/radiobutton.cpp
widgets/container.cpp
widgets/tab.cpp
widgets/icon.cpp
widgets/scrollarea.cpp
widgets/label.cpp
widgets/button.cpp
widgets/checkbox.cpp
widgets/textbox.cpp
widgets/window.cpp
widgets/slider.cpp
widgets/tabbedarea.cpp
)

set(COMMON
keyinput.cpp
basiccontainer.cpp
genericinput.cpp
cliprectangle.cpp
defaultfont.cpp
image.cpp
keyevent.cpp
focushandler.cpp
mouseinput.cpp
inputevent.cpp
key.cpp
widget.cpp
color.cpp
font.cpp
mouseevent.cpp
gui.cpp
exception.cpp
guichan.cpp
graphics.cpp
imagefont.cpp
rectangle.cpp
actionevent.cpp
event.cpp
selectionevent.cpp
)

set(SDL
sdl/sdlgraphics.cpp
sdl/sdlimageloader.cpp
sdl/sdlinput.cpp
sdl/sdlimage.cpp
sdl/sdl.cpp
)

set(HGE
hge/hgeimage.cpp
hge/hgeinput.cpp
hge/hgeimagefont.cpp
hge/hgeimageloader.cpp
hge/hgegraphics.cpp
)

set(OGL
opengl/openglgraphics.cpp
opengl/opengl.cpp
opengl/openglimage.cpp
)

set(OPENLAYER
openlayer/openlayerimageloader.cpp
openlayer/openlayer.cpp
openlayer/openlayerimage.cpp
openlayer/openlayergraphics.cpp
openlayer/openlayerinput.cpp
openlayer/openlayerfont.cpp
)

set(CONTRIB
contrib/opengl/oglftfont.cpp
contrib/sdl/sdltruetypefont.cpp
contrib/allegro/allegroglyphkeeperfont.cpp
contrib/widgets/adjustingcontainer.cpp
)


add_library(guichan ${COMMON} ${SDL} ${WIDGETS})


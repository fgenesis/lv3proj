file(GLOB lzo_SRC lzo/*.c)

set(shared_SRC
AnimParser.cpp
AppFalcon.cpp
AsciiLevelParser.cpp
MyCrc32.cpp
Engine.cpp
FalconBaseModule.cpp
FalconObjectModule.cpp
IMG_savepng.c
LayerMgr.cpp
log.cpp
LVPAFile.cpp
LVPAStreamCipher.cpp
LZOCompressor.cpp
LZMACompressor.cpp
MemoryLeaks.cpp
MapFile.cpp
ObjectMgr.cpp
Objects.cpp
PhysicsSystem.cpp
ProgressBar.cpp
PropParser.cpp
ResourceMgr.cpp
SDL_func.cpp
SHA256Hash.cpp
sha256.cpp
SoundCore.cpp
Tile.cpp
TileLayer.cpp
tools.cpp
VFSDir.cpp
VFSDirLVPA.cpp
VFSFile.cpp
VFSFileLVPA.cpp
VFSHelper.cpp
VFSLoader.cpp
VFSLoaderLVPA.cpp

# SDL_gfx files
SDL_gfx/SDL_framerate.c
SDL_gfx/SDL_gfxPrimitives.c
SDL_gfx/SDL_imageFilter.c
SDL_gfx/SDL_rotozoom.c

# LZMA compression lib
lzma/LzFind.c
lzma/Lzma2Dec.c
lzma/Lzma2Enc.c
lzma/LzmaDec.c
lzma/LzmaEnc.c

# LZO compression lib
${lzo_SRC}
)

include_directories (${TOP_INCLUDE_DIR})
include_directories ("${SRC_ROOT_DIR}/gme") # only shared needs this
add_library(shared ${shared_SRC})
target_link_libraries(shared ${SDL_ALL_LIBS} libpng gme)

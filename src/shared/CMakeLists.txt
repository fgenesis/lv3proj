set(shared_SRC
AnimParser.cpp
AppFalcon.cpp
AsciiLevelParser.cpp
Crc32.cpp
Engine.cpp
FalconBaseModule.cpp
FalconObjectModule.cpp
LayerMgr.cpp
log.cpp
LVPAFile.cpp
LVPAFileStore.cpp
LZMACompressor.cpp
ObjectMgr.cpp
Objects.cpp
PhysicsSystem.cpp
PropParser.cpp
ResourceMgr.cpp
SDL_func.cpp
SoundCore.cpp
Tile.cpp
TileLayer.cpp
tools.cpp
IMG_savepng.c
LzFind.c
Lzma2Dec.c
Lzma2Enc.c
LzmaDec.c
LzmaEnc.c

)
include_directories (${LOSTVIKINGS3_SOURCE_DIR}/src/include) 
add_library(shared ${shared_SRC})
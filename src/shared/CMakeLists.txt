set(shared_SRC
AnimParser.cpp
AppFalcon.cpp
AsciiLevelParser.cpp
Crc32.cpp
Engine.cpp
FalconBaseModule.cpp
FalconObjectModule.cpp
IMG_savepng.c
LayerMgr.cpp
log.cpp
LVPAFile.cpp
LzFind.c
Lzma2Dec.c
Lzma2Enc.c
LZMACompressor.cpp
LzmaDec.c
LzmaEnc.c
MapFile.cpp
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
VFSDir.cpp
VFSDirLVPA.cpp
VFSFile.cpp
VFSFileLVPA.cpp
VFSHelper.cpp
)

include_directories (${TOP_INCLUDE_DIR}) 
add_library(shared ${shared_SRC})
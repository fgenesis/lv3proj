####################################################################
# FALCON - The Falcon Programming Language.
#
# CMake configuration file for bufext
####################################################################


include_directories(
  ${TOP_INCLUDE_DIR}
)


# Sources files the module is built from.
set(SRC_FILES 
  bufext.cpp
  bufext_ext.cpp
  bufext_st.cpp
)

# These are actually not needed by cmake to build. But if omitted they won't be
# listed in the virtual file tree of Visual Studio.
set(HDR_FILES
  bufext_ext.h
  bufext_st.h
  bytebuf.h
  bitbuf.h
  endianswap.h
  buffererror.h
)


# Builds and links from the source files
add_library(bufext_fm
   ${SRC_FILES}
   ${HDR_FILES}
)

target_link_libraries(bufext_fm falcon_engine)

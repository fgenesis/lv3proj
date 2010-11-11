####################################################################
# The Falcon Programming language
#
# CMake configuration file for module Compiler
####################################################################

include_directories(
  ${TOP_INCLUDE_DIR}
)

add_library(compiler_fm
  compiler.cpp
  compiler_ext.cpp
  compiler_mod.cpp
  compiler_st.cpp
)

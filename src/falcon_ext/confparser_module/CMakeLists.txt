####################################################################
# The Falcon Programming language
#
# CMake configuration file for module ConfParser
####################################################################

include_directories(
  ${TOP_INCLUDE_DIR}
)

add_library(confparser_fm
  confparser.cpp
  confparser_ext.cpp
  confparser_mod.cpp
  confparser_st.cpp
)

target_link_libraries(confparser_fm falcon_engine)

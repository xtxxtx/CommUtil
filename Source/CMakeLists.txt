include_directories(${PROJECT_SOURCE_DIR}/Include)
message(STATUS "Include: " ${PROJECT_SOURCE_DIR}/Include)
set(SRC_LST Log.cpp)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Lib)
add_library(CommUtil ${SRC_LST})


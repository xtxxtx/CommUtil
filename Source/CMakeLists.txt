include_directories(${PROJECT_SOURCE_DIR}/Include)
message(STATUS "Include: " ${PROJECT_SOURCE_DIR}/Include)

set(CMAKE_C_FLAGS "-g -Wall")
set(CMAKE_CXX_FLAGS "-g -Wall")

#aux_source_directory(. SRC_LIST)

set(SRC_LST 
	OSPath.cpp
	OSTime.cpp
	Log.cpp
	Thread.cpp
	ThreadPool.cpp
	PacketManager.cpp
	SocketHandle.cpp
#	SocketListen.cpp
	SocketClient.cpp
	SocketServer.cpp)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Lib)
add_library(CommUtil ${SRC_LST})
target_link_libraries(CommUtil pthread)


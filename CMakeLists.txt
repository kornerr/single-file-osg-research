CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(SFOSG)
SET(BINARY_NAME "sfosg")

# Build OpenSceneGraph as part of OGS.
#INCLUDE(${OGS_DIR}/${STAGE}/OpenSceneGraph/CMakeLists.txt)

# Create and link executable.
ADD_EXECUTABLE(${BINARY_NAME} main.cpp)
#TARGET_LINK_LIBRARIES(
#    ${BINARY_NAME}
#    osgViewer
#    osgUtil
#    osgDB
#    osgGA
#    osg
#    OpenThreads
#)
# Use C++14.
TARGET_COMPILE_OPTIONS(${BINARY_NAME} PUBLIC "-std=c++14")

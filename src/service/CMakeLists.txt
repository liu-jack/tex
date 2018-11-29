ADD_SUBDIRECTORY(sdp)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/sdp)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_FILES)
use_sdp_files(mfw-service SOURCE_FILES)
use_sdp_files(mfw-log SOURCE_FILES)
use_sdp_files(mfw-registry SOURCE_FILES)

ADD_LIBRARY(service STATIC ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(service util pthread)
add_sdp_dependency(service mfw-service mfw-registry mfw-log)

INSTALL(TARGETS service DESTINATION lib EXPORT mfw-libs)

FILE(GLOB headers *.h)
INSTALL(FILES ${headers} DESTINATION include/service)

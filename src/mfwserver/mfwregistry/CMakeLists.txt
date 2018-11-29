INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src/service/sdp)
INCLUDE(${PROJECT_SOURCE_DIR}/src/cmake/mysql.cmake)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} SOURCE_FILES)

ADD_EXECUTABLE(mfwregistry ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(mfwregistry service util mysql rt dl)

INSTALL(TARGETS mfwregistry DESTINATION server/mfwregistry)
INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/start.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/stop.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/mfwregistry.mfw.conf.example
    ${CMAKE_CURRENT_SOURCE_DIR}/mfwregistry.conf.example
    DESTINATION server/mfwregistry)

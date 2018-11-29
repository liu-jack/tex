
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/test)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/3party/jsoncpp)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/3party/utf8)

INCLUDE(${PROJECT_SOURCE_DIR}/src/cmake/mysql.cmake)

ADD_EXECUTABLE(test_http test_http.cpp)
TARGET_LINK_LIBRARIES(test_http util pthread curl)

ADD_EXECUTABLE(test_urlencode test_urlencode.cpp)
TARGET_LINK_LIBRARIES(test_urlencode util pthread curl)

ADD_EXECUTABLE(test_string test_string.cpp)
TARGET_LINK_LIBRARIES(test_string util pthread)

ADD_EXECUTABLE(test_json test_json.cpp)
TARGET_LINK_LIBRARIES(test_json util pthread jsoncpp)

ADD_EXECUTABLE(test_utf8 test_utf8.cpp)
TARGET_LINK_LIBRARIES(test_utf8 util)

ADD_EXECUTABLE(test_sdp test_sdp.cpp)
TARGET_LINK_LIBRARIES(test_sdp util)

ADD_EXECUTABLE(test_lua test_lua.cpp)
TARGET_LINK_LIBRARIES(test_lua luamod util luajit pthread dl)

ADD_EXECUTABLE(test_mysql test_mysql.cpp)
TARGET_LINK_LIBRARIES(test_mysql util mysql pthread dl rt)

ADD_SUBDIRECTORY(websocket)

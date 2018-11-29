INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src/service/sdp)

ADD_LIBRARY(redis Redis.cpp RedisWrapper.cpp)
add_sdp_dependency(redis mfw-service)

INSTALL(TARGETS redis DESTINATION lib EXPORT mfw-libs)
INSTALL(FILES Redis.h RedisWrapper.h DESTINATION include/redis)

ADD_CUSTOM_COMMAND(
    OUTPUT lex.yy.hpp lex.yy.cpp
    COMMAND flex -o lex.yy.cpp --header-file=lex.yy.hpp ${CMAKE_CURRENT_SOURCE_DIR}/lexical.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lexical.l
)

ADD_CUSTOM_COMMAND(
    OUTPUT syntax.tab.hpp syntax.tab.cpp
    COMMAND bison -d -o syntax.tab.cpp ${CMAKE_CURRENT_SOURCE_DIR}/syntax.y
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/syntax.y
)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(parse STATIC lex.yy.hpp lex.yy.cpp syntax.tab.hpp syntax.tab.cpp sdptree.cpp parse.cpp)
TARGET_LINK_LIBRARIES(parse util)

INSTALL(TARGETS parse DESTINATION lib EXPORT mfw-libs)
INSTALL(FILES parse.h sdptree.h DESTINATION include/parse)

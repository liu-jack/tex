ADD_LIBRARY(mysql STATIC IMPORTED)
SET_TARGET_PROPERTIES(mysql PROPERTIES IMPORTED_LOCATION /usr/local/mysql/lib/libmysqlclient.a)

INCLUDE_DIRECTORIES(/usr/local/mysql/include)

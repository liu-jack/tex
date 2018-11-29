#----------------------------------------------------------------
# Generated CMake target import file for configuration "".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "util" for configuration ""
set_property(TARGET util APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(util PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C;CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libutil.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS util )
list(APPEND _IMPORT_CHECK_FILES_FOR_util "${_IMPORT_PREFIX}/lib/libutil.a" )

# Import target "parse" for configuration ""
set_property(TARGET parse APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(parse PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "util"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libparse.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS parse )
list(APPEND _IMPORT_CHECK_FILES_FOR_parse "${_IMPORT_PREFIX}/lib/libparse.a" )

# Import target "sdp2cpp" for configuration ""
set_property(TARGET sdp2cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sdp2cpp PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/sdp2cpp"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdp2cpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdp2cpp "${_IMPORT_PREFIX}/bin/sdp2cpp" )

# Import target "sdp2lua" for configuration ""
set_property(TARGET sdp2lua APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sdp2lua PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/sdp2lua"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdp2lua )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdp2lua "${_IMPORT_PREFIX}/bin/sdp2lua" )

# Import target "sdplua" for configuration ""
set_property(TARGET sdplua APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sdplua PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lua/libsdplua.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdplua )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdplua "${_IMPORT_PREFIX}/lua/libsdplua.a" )

# Import target "sdp2luadoc" for configuration ""
set_property(TARGET sdp2luadoc APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sdp2luadoc PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/sdp2luadoc"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdp2luadoc )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdp2luadoc "${_IMPORT_PREFIX}/bin/sdp2luadoc" )

# Import target "sdp2php" for configuration ""
set_property(TARGET sdp2php APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sdp2php PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/sdp2php"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdp2php )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdp2php "${_IMPORT_PREFIX}/bin/sdp2php" )

# Import target "sdp2js" for configuration ""
set_property(TARGET sdp2js APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sdp2js PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/sdp2js"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdp2js )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdp2js "${_IMPORT_PREFIX}/bin/sdp2js" )

# Import target "service" for configuration ""
set_property(TARGET service APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(service PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "util;pthread"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libservice.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS service )
list(APPEND _IMPORT_CHECK_FILES_FOR_service "${_IMPORT_PREFIX}/lib/libservice.a" )

# Import target "redis" for configuration ""
set_property(TARGET redis APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(redis PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libredis.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS redis )
list(APPEND _IMPORT_CHECK_FILES_FOR_redis "${_IMPORT_PREFIX}/lib/libredis.a" )

# Import target "jsoncpp" for configuration ""
set_property(TARGET jsoncpp APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(jsoncpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/3party-lib/libjsoncpp.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS jsoncpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_jsoncpp "${_IMPORT_PREFIX}/3party-lib/libjsoncpp.a" )

# Import target "zlib" for configuration ""
set_property(TARGET zlib APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(zlib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/3party-lib/libzlib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS zlib )
list(APPEND _IMPORT_CHECK_FILES_FOR_zlib "${_IMPORT_PREFIX}/3party-lib/libzlib.a" )

# Import target "tinyxml" for configuration ""
set_property(TARGET tinyxml APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(tinyxml PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/3party-lib/libtinyxml.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS tinyxml )
list(APPEND _IMPORT_CHECK_FILES_FOR_tinyxml "${_IMPORT_PREFIX}/3party-lib/libtinyxml.a" )

# Import target "tolua++" for configuration ""
set_property(TARGET tolua++ APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(tolua++ PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/tolua++"
  )

list(APPEND _IMPORT_CHECK_TARGETS tolua++ )
list(APPEND _IMPORT_CHECK_FILES_FOR_tolua++ "${_IMPORT_PREFIX}/bin/tolua++" )

# Import target "luamod" for configuration ""
set_property(TARGET luamod APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(luamod PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "sdplua"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libluamod.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS luamod )
list(APPEND _IMPORT_CHECK_FILES_FOR_luamod "${_IMPORT_PREFIX}/lib/libluamod.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

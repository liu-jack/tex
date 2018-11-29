INSTALL(DIRECTORY mfw DESTINATION php)

SET(MfwPacket_Php ${CMAKE_CURRENT_BINARY_DIR}/mfw/MfwPacket.php)
SET(MfwPacket_Sdp ${PROJECT_SOURCE_DIR}/src/service/sdp/service/MfwPacket.sdp)

SET(MfwQuery_Php ${CMAKE_CURRENT_BINARY_DIR}/mfw/Query.php)
SET(MfwQuery_Sdp ${PROJECT_SOURCE_DIR}/src/service/sdp/registry/Query.sdp)

ADD_CUSTOM_COMMAND(
    OUTPUT ${MfwPacket_Php}
    COMMAND sdp2php --dir=${CMAKE_CURRENT_BINARY_DIR}/mfw ${MfwPacket_Sdp}
    DEPENDS ${MfwPacket_Sdp}
)

ADD_CUSTOM_COMMAND(
    OUTPUT ${MfwQuery_Php}
    COMMAND sdp2php --dir=${CMAKE_CURRENT_BINARY_DIR}/mfw ${MfwQuery_Sdp}
    DEPENDS ${MfwQuery_Sdp}
)

ADD_CUSTOM_TARGET(sdpphp-inc ALL DEPENDS ${MfwPacket_Php} ${MfwQuery_Php})

INSTALL(FILES ${MfwPacket_Php} ${MfwQuery_Php} DESTINATION php/mfw)

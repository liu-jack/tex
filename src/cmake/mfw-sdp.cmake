FUNCTION(build_sdp name srcdir)
    SET(NO-LIBRARY)
    IF ("${ARGV2}" STREQUAL "NO-LIBRARY")
        SET(NO-LIBRARY 1)
    ENDIF()

    FILE(RELATIVE_PATH bindir ${CMAKE_SOURCE_DIR} ${srcdir})
    SET(bindir "${CMAKE_BINARY_DIR}/${bindir}")
    MESSAGE(STATUS "build_sdp: ${name} ${srcdir} ${bindir}")
    
    FILE(GLOB sdpfiles RELATIVE ${srcdir} ${srcdir}/*.sdp)
    IF(NOT sdpfiles)
        RETURN()
    ENDIF()
    
    SET(sdp_all_definition_files)
    SET(sdp_all_generated_header_files)
    SET(sdp_all_generated_source_files)
    FOREACH(sdpfile ${sdpfiles})
        STRING(REGEX REPLACE "\\.sdp" "" file ${sdpfile})
        EXECUTE_PROCESS(COMMAND grep -wl interface ${srcdir}/${file}.sdp OUTPUT_VARIABLE has_interface)
        SET(sdp_generate_header_files ${bindir}/${file}.h)
        SET(sdp_generate_source_files)
        IF(NOT "${has_interface}" STREQUAL "")
            SET(sdp_generate_source_files ${bindir}/${file}.cpp)
        ENDIF()
        
        ADD_CUSTOM_COMMAND(
            OUTPUT ${sdp_generate_header_files} ${sdp_generate_source_files}
            COMMAND sdp2cpp --dir=${bindir} ${srcdir}/${file}.sdp
            DEPENDS ${srcdir}/${file}.sdp
        )

        LIST(APPEND sdp_all_definition_files ${srcdir}/${file}.sdp)
        LIST(APPEND sdp_all_generated_header_files ${sdp_generate_header_files})
        LIST(APPEND sdp_all_generated_source_files ${sdp_generate_source_files})
    ENDFOREACH()
    
    SET_PROPERTY(GLOBAL PROPERTY sdp-definition-${name} ${sdp_all_definition_files})
    SET_PROPERTY(GLOBAL PROPERTY sdp-headers-${name} ${sdp_all_generated_header_files})
    SET_PROPERTY(GLOBAL PROPERTY sdp-files-${name} ${sdp_all_generated_header_files} ${sdp_all_generated_source_files})
    IF (sdp_all_generated_source_files AND (NOT NO-LIBRARY))
        ADD_LIBRARY(sdp-${name} STATIC ${sdp_all_generated_header_files} ${sdp_all_generated_source_files})
    ELSE()
        ADD_CUSTOM_TARGET(sdp-${name} DEPENDS ${sdp_all_generated_header_files} ${sdp_all_generated_source_files})
    ENDIF()
ENDFUNCTION()

FUNCTION(use_sdp_library name library_files_var)
    SET(${library_files_var} ${${library_files_var}} sdp-${name} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(use_sdp_files name source_files_var)
    GET_PROPERTY(source_files GLOBAL PROPERTY sdp-files-${name})
    SET_SOURCE_FILES_PROPERTIES(${source_files} PROPERTIES GENERATED 1)
    SET(${source_files_var} ${${source_files_var}} ${source_files} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(add_sdp_dependency target_name name)
    ADD_DEPENDENCIES(${target_name} sdp-${name})
    FOREACH(arg ${ARGN})
    	ADD_DEPENDENCIES(${target_name} sdp-${arg})
    ENDFOREACH()
ENDFUNCTION()

FUNCTION(install_sdp name)
    STRING(REGEX REPLACE "-.*" "" app ${name})
    STRING(REGEX REPLACE "[^-]*-" "" server ${name})
    INSTALL(TARGETS sdp-${name} DESTINATION /usr/local/mfwsdp/${app}/${server} EXPORT mfwsdp-${app})
    
    GET_PROPERTY(definition_files GLOBAL PROPERTY sdp-definition-${name})
    INSTALL(FILES ${definition_files} DESTINATION /usr/local/mfwsdp/${app}/${server})
    
    GET_PROPERTY(source_files GLOBAL PROPERTY sdp-files-${name})
    INSTALL(FILES ${source_files} DESTINATION /usr/local/mfwsdp/${app}/${server})
ENDFUNCTION()

FUNCTION(export_sdp app)
    INSTALL(EXPORT mfwsdp-${app} DESTINATION /usr/local/mfwsdp)
ENDFUNCTION()

FUNCTION(import_sdp_library name library_files_var)
    STRING(REGEX REPLACE "-.*" "" app ${name})
    IF (NOT sdp-imported-flag-${app})
        INCLUDE(/usr/local/mfwsdp/mfwsdp-${app}.cmake)
        SET(sdp-imported-flag-${app} 1 PARENT_SCOPE)
    ENDIF()

    SET(${library_files_var} ${${library_files_var}} sdp-${name} PARENT_SCOPE)
ENDFUNCTION()

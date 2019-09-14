# ==============================================================================
# WOLF CMake File
# ==============================================================================

INCLUDE_DIRECTORIES(wolf-stat-server)
INCLUDE_DIRECTORIES(wolf-stat-client)

IF( ${STAT_TARGET} STREQUAL "server" )
    ADD_SUBDIRECTORY(wolf-stat-server)
ELSEIF(${STAT_TARGET} STREQUAL "client" )
    ADD_SUBDIRECTORY(wolf-stat-client)
ELSE()
    ADD_SUBDIRECTORY(wolf-stat-server)
    ADD_SUBDIRECTORY(wolf-stat-client)
ENDIF()

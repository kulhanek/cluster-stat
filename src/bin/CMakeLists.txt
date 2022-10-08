# ==============================================================================
# Cluster CMake File
# ==============================================================================

INCLUDE_DIRECTORIES(cluster-stat-server)
INCLUDE_DIRECTORIES(cluster-stat-client)

IF( ${STAT_TARGET} STREQUAL "server" )
    ADD_SUBDIRECTORY(cluster-stat-server)
ELSEIF(${STAT_TARGET} STREQUAL "client" )
    ADD_SUBDIRECTORY(cluster-stat-client)
ELSE()
    ADD_SUBDIRECTORY(cluster-stat-server)
    ADD_SUBDIRECTORY(cluster-stat-client)
ENDIF()

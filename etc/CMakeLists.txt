# ==============================================================================
# WOLF CMake File
# ==============================================================================

IF( (${STAT_TARGET} STREQUAL "server") OR (${STAT_TARGET} STREQUAL "both") )
    INSTALL(FILES
                wolf-stat-server.xml
           DESTINATION
                etc
            PERMISSIONS
                OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
            )
ENDIF()

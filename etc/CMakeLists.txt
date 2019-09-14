# ==============================================================================
# WOLF CMake File
# ==============================================================================

IF( (${STAT_TARGET} STREQUAL "server") OR (${STAT_TARGET} STREQUAL "both") )
    INSTALL(FILES
                wolf-stat-server.xml
           DESTINATION
                etc
            PERMISSIONS
                OWNER_READ OWNER_WRITE GROUP_READ
            )
    INSTALL(FILES
                wolf-stat-server.service
           DESTINATION
                etc
            PERMISSIONS
                OWNER_READ OWNER_WRITE GROUP_READ
            )
ENDIF()

# ==============================================================================
# AMS CMake File
# ==============================================================================

# program objects --------------------------------------------------------------
SET(STAT_SRC
        StatClient.cpp
        ClientOptions.cpp
        ../wolf-stat-server/StatDatagram.cpp
        ../wolf-stat-server/StatMainHeader.cpp
        )

# final build ------------------------------------------------------------------
ADD_EXECUTABLE(wolf-stat-client ${STAT_SRC})

TARGET_LINK_LIBRARIES(wolf-stat-client ${WOLF_LIBS})

INSTALL(TARGETS
            wolf-stat-client
        DESTINATION
            bin
        )


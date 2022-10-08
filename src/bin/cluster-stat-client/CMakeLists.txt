# ==============================================================================
# AMS CMake File
# ==============================================================================

# program objects --------------------------------------------------------------
SET(STAT_SRC
        StatClient.cpp
        ClientOptions.cpp
        ../cluster-stat-server/StatDatagram.cpp
        ../cluster-stat-server/StatMainHeader.cpp
        )

# final build ------------------------------------------------------------------
ADD_EXECUTABLE(cluster-stat-client ${STAT_SRC})

TARGET_LINK_LIBRARIES(cluster-stat-client ${STAT_LIBS})

INSTALL(TARGETS
            cluster-stat-client
        DESTINATION
            bin
        )


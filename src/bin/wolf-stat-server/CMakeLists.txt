# ==============================================================================
# WOLF CMake File
# ==============================================================================

# program objects --------------------------------------------------------------
SET(PROG_SRC
        WolfStatOptions.cpp
        WolfStatServer.cpp
        ServerWatcher.cpp
        prefix.c
        _ListLoggedUsers.cpp
        _Error.cpp
        )

# final build ------------------------------------------------------------------
ADD_EXECUTABLE(wolf-stat-server ${PROG_SRC})

TARGET_LINK_LIBRARIES(wolf-stat-server ${WOLF_LIBS})

INSTALL(TARGETS
            wolf-stat-server
        RUNTIME DESTINATION
            bin
        )


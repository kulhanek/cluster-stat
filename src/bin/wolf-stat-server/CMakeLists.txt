# ==============================================================================
# WOLF CMake File
# ==============================================================================

# program objects --------------------------------------------------------------
SET(PROG_SRC
        prefix.c
        StatMainHeader.cpp
        ServerOptions.cpp
        FCGIStatServer.cpp
        StatServer.cpp
        StatDatagram.cpp
        ServerWatcher.cpp
        _ListLoggedUsers.cpp
        _Debug.cpp
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


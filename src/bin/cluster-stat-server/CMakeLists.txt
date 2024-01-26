# ==============================================================================
# Cluster CMake File
# ==============================================================================

INCLUDE_DIRECTORIES(batchsys)
INCLUDE_DIRECTORIES(batchsys/pbspro)

# program objects --------------------------------------------------------------
SET(PROG_SRC
        StatMainHeader.cpp
        ServerOptions.cpp
        FCGIStatServer.cpp
        StatServer.cpp
        StatDatagram.cpp
        ServerWatcher.cpp
        _ListLoggedUsers.cpp
        _ListAllSeats.cpp
        _RemoteAccess.cpp
        _Debug.cpp
        _Error.cpp
        batchsys/PBSProAttr.cpp
        batchsys/PBSProServer.cpp
        BatchSystemWatcher.cpp
        )

# final build ------------------------------------------------------------------
ADD_EXECUTABLE(cluster-stat-server ${PROG_SRC})

TARGET_LINK_LIBRARIES(cluster-stat-server ${STAT_LIBS})

INSTALL(TARGETS
            cluster-stat-server
        RUNTIME DESTINATION
            bin
        )


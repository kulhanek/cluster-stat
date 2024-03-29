# ==============================================================================
# ClusterStat CMake File
# ==============================================================================

CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)
#SET(CMAKE_VERBOSE_MAKEFILE ON)

PROJECT(CLusterStat)

# ==============================================================================
# conditional setup ------------------------------------------------------------
# ==============================================================================

SET(LIBS_STATIC OFF     CACHE BOOL "Should the static version of hipoly library be built?")
SET(LIBS_SHARED ON      CACHE BOOL "Should the dynamic version of hipoly library be built?")
SET(TRY_QT_LIB  ON      CACHE BOOL "Should the QT lib be used?")
SET(STAT_TARGET "both"  CACHE STRING "targets: server, client, both")

# ==============================================================================
# project setup ----------------------------------------------------------------
# ==============================================================================

# define project name ----------------------------
SET(CLUSTER_STAT_VERSION 3.0.x)

# what languages we use --------------------------
ENABLE_LANGUAGE(CXX)

# ==============================================================================
# external dependencies --------------------------------------------------------
# ==============================================================================

IF(NOT DEFINED COMPOSITE_PROJECT)
    SET(DEVELOPMENT_ROOT "$ENV{DEVELOPMENT_ROOT}")
ENDIF(NOT DEFINED COMPOSITE_PROJECT)

IF(NOT DEVELOPMENT_ROOT)
    MESSAGE(FATAL_ERROR "DEVELOPMENT_ROOT variable is not set!")
ENDIF(NOT DEVELOPMENT_ROOT)

# ==============================================================================
# external libraries -----------------------------------------------------------
# ==============================================================================

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${DEVELOPMENT_ROOT}/projects/cmake-modules/1.0/share/modules")

# boost =======================
FIND_PACKAGE(MyBoost REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS} SYSTEM)

# HIPOLY -----------------------------------------
SET(HIPOLY_ROOT ${DEVELOPMENT_ROOT}/projects/hipoly/1.0)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/compress SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/error SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/event SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/file SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/list SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/option SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/string SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/stream SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/thread SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/time SYSTEM)
INCLUDE_DIRECTORIES(${HIPOLY_ROOT}/src/lib/hipoly/xml SYSTEM)
LINK_DIRECTORIES(${HIPOLY_ROOT}/lib)
SET(HIPOLY_LIB_NAME hipoly)

# W3TK -------------------------------------------
SET(W3TK_ROOT ${DEVELOPMENT_ROOT}/projects/w3tk/1.0)
INCLUDE_DIRECTORIES(${W3TK_ROOT}/src/lib/w3tk SYSTEM)
INCLUDE_DIRECTORIES(${W3TK_ROOT}/src/lib/w3tk/fastcgi SYSTEM)
INCLUDE_DIRECTORIES(${W3TK_ROOT}/src/lib/w3tk/template SYSTEM)
LINK_DIRECTORIES(${W3TK_ROOT}/lib)
SET(W3TK_LIB_NAME w3tk)

# all libs ---------------------------------------
SET(STAT_LIBS
        ${W3TK_LIB_NAME}
        ${HIPOLY_LIB_NAME}
        )

# architecture -----------------------------------
SET(UNIX 1)
ADD_DEFINITIONS(-DUNIX)

# policies ---------------------------------------
IF(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

# ==============================================================================
# output directories -----------------------------------------------------------
# ==============================================================================

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# rpaths -----------------------------------------------------------------------
# ==============================================================================

IF(NOT DEFINED COMPOSITE_PROJECT)
    # use, i.e. don't skip the full RPATH for the build tree
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    # the RPATH to be used when installing
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
ENDIF(NOT DEFINED COMPOSITE_PROJECT)

# ==============================================================================
# compiler setups --------------------------------------------------------------
# ==============================================================================

IF(NOT DEFINED COMPOSITE_PROJECT)
    SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
    SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Werror -pedantic-errors -Wundef -Wno-long-long")

    #SET(CMAKE_BUILD_TYPE Release)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF(NOT DEFINED COMPOSITE_PROJECT)

# ==============================================================================
# project subdirectories  ------------------------------------------------------
# ==============================================================================

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(etc)






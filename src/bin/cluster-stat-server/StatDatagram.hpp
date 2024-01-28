#ifndef StatDatagramH
#define StatDatagramH
// =============================================================================
// cluster-stat-client
// -----------------------------------------------------------------------------
//    Copyright (C) 2021      Petr Kulhanek, kulhanek@chemi.muni.cz
//    Copyright (C) 2019      Petr Kulhanek, kulhanek@chemi.muni.cz
//
//     This program is free software; you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation; either version 2 of the License, or
//     (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License along
//     with this program; if not, write to the Free Software Foundation, Inc.,
//     51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
// =============================================================================

#include <SmallString.hpp>
#include <ostream>

// -----------------------------------------------------------------------------

#define HEADER_SIZE 4
#define NAME_SIZE   64
#define MAX_TTYS    12
#define BUFFER_LEN  1024

// -----------------------------------------------------------------------------

class CStatDatagram {
public:
    CStatDatagram(void);

// setters ---------------------------------------------------------------------
    void SetDatagram(bool powerdown=false);
    void SetNodeName(const CSmallString& name);
    void Clear(void);

// getters ---------------------------------------------------------------------
    CSmallString GetNodeName(void);             // node name is always short

    CSmallString GetLocalUserName(void);
    CSmallString GetLocalLoginName(void);
    char         GetLocalLoginType(void);

    CSmallString GetLocalUserName(int id);
    CSmallString GetLocalLoginName(int id);
    char         GetLocalLoginType(int id);

    CSmallString GetRemoteUserName(int id);
    CSmallString GetRemoteLoginName(int id);
    char         GetRemoteLoginType(int id);
    CSmallString GetRemoteDisplayID(int id);

    int          GetTimeStamp(void);
    bool         IsDown(void);
    void         PrintInfo(std::ostream& vout);
    bool         IsValid(void);

// private data ----------------------------------------------------------------
private:
    char    Header[HEADER_SIZE];
    char    NodeName[NAME_SIZE];
    // local users
    int     NumOfLocalUsers;
    char    LocalUserName[MAX_TTYS][NAME_SIZE];
    char    LocalLoginName[MAX_TTYS][NAME_SIZE];
    char    LocalLoginType[MAX_TTYS];

    char    ActiveLocalUserName[NAME_SIZE];
    char    ActiveLocalLoginName[NAME_SIZE];
    char    ActiveLocalLoginType;

    // remote users
    int     NumOfRemoteUsers;
    int     NumOfVNCRemoteUsers;    // subgroup of NumOfRemoteUsers
    int     NumOfRDSKRemoteUsers;   // subgroup of NumOfRemoteUsers
    char    RemoteUserName[MAX_TTYS][NAME_SIZE];
    char    RemoteLoginName[MAX_TTYS][NAME_SIZE];
    char    RemoteLoginType[MAX_TTYS];
    char    RemoteDisplayID[MAX_TTYS][NAME_SIZE];
    // service information
    int     PowerDown;
    int     TimeStamp;                      // time of "meassurement"
    int     CheckSum;

    friend class CFCGIStatServer;
};

// -----------------------------------------------------------------------------

class CUserSession {
public:
    std::string SessionID;
    char        UserName[NAME_SIZE];
    char        LoginName[NAME_SIZE];
};

// -----------------------------------------------------------------------------

#endif

#ifndef StatDatagramH
#define StatDatagramH
// =============================================================================
// wolf-stat-client
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------

class CStatDatagram {
public:
    CStatDatagram(void);

// setters ---------------------------------------------------------------------
    void SetDatagram(void);

// getters ---------------------------------------------------------------------
    CSmallString GetShortNodeName(void);
    CSmallString GetNodeName(void);
    CSmallString GetUserName(void);
    CSmallString GetLoginName(void);
    int          GetTimeStamp(void);
    void         PrintInfo(std::ostream& vout);
    bool         IsValid(void);

// private data ----------------------------------------------------------------
private:
    char    Header[HEADER_SIZE];
    char    NodeName[NAME_SIZE];
    char    UserName[NAME_SIZE];
    char    LoginName[NAME_SIZE];
    int     IUsers;                 // number of interactive user sessions
    int     RUsers;                 // number of all sessions
    int     TimeStamp;              // time of "meassurement"
    int     CheckSum;
};

// -----------------------------------------------------------------------------

#endif

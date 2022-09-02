// =============================================================================
//  WOLF Stat Server
// -----------------------------------------------------------------------------
//     Copyright (C) 2020 Petr Kulhanek (kulhanek@chemi.muni.cz)
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

#include "FCGIStatServer.hpp"
#include <TemplateParams.hpp>
#include <ErrorSystem.hpp>
#include <FileName.hpp>
#include <SmallTimeAndDate.hpp>
#include <SmallTime.hpp>
#include <DirectoryEnum.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace boost;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CFCGIStatServer::_ListAllSeats(CFCGIRequest& request)
{
    NodesMutex.Lock();

    std::map<std::string,CCompNode>::iterator it = Nodes.begin();
    std::map<std::string,CCompNode>::iterator ie = Nodes.end();

    while( it != ie ){
        CStatDatagram dtg = it->second.Basic;

        // check node status
        CSmallString status = "up";
        // check timestamp from user stat file
        CSmallTimeAndDate stime(dtg.GetTimeStamp());
        CSmallTimeAndDate ctime;
        ctime.GetActualTimeAndDate();
        CSmallTime diff = ctime - stime;
        if( (diff > 180) || dtg.IsDown() ){  // skew of 300 seconds
            status = "down";
        }

        // write response
        request.OutStream.PutStr(status); // node status
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(dtg.GetShortNodeName()); // node name

        request.OutStream.PutChar(';');
        request.OutStream.PutStr(CSmallString(dtg.NumOfLocalUsers));
        request.OutStream.PutChar(',');
        request.OutStream.PutStr(CSmallString(dtg.NumOfRemoteUsers));
        request.OutStream.PutChar(',');
        request.OutStream.PutStr(CSmallString(dtg.NumOfVNCRemoteUsers));

        if( (dtg.NumOfLocalUsers > 0) || (dtg.NumOfRemoteUsers > 0) ){
            request.OutStream.PutChar(';');
        }
        bool delimit = false;
        for(int i=0; i < dtg.NumOfLocalUsers; i++){
            if( delimit ) request.OutStream.PutStr("|");
            request.OutStream.PutStr(dtg.GetLocalUserName(i));
            request.OutStream.PutStr(" (");
            request.OutStream.PutStr(dtg.GetLocalLoginName(i));
            request.OutStream.PutStr(") [X11]");
            delimit = true;
        }
        for(int i=0; i < dtg.NumOfRemoteUsers; i++){
            if( delimit ) request.OutStream.PutStr("|");
            request.OutStream.PutStr(dtg.GetRemoteUserName(i));
            request.OutStream.PutStr(" (");
            request.OutStream.PutStr(dtg.GetRemoteLoginName(i));
            if( dtg.GetRemoteLoginType(i) == 'V' ){
                request.OutStream.PutStr(") [VNC]");
            } else {
                request.OutStream.PutStr(") [ssh]");
            }
            delimit = true;
        }

        request.OutStream.PutChar('\n');

        it++;
    }

    NodesMutex.Unlock();

    // finalize request
    request.FinishRequest();

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

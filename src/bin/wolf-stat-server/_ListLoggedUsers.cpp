// =============================================================================
//  WOLF Stat Server
// -----------------------------------------------------------------------------
//     Copyright (C) 2015 Petr Kulhanek (kulhanek@chemi.muni.cz)
//     Copyright (C) 2012 Petr Kulhanek (kulhanek@chemi.muni.cz)
//     Copyright (C) 2011      Petr Kulhanek, kulhanek@chemi.muni.cz
//     Copyright (C) 2001-2008 Petr Kulhanek, kulhanek@chemi.muni.cz
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

bool CFCGIStatServer::_ListLoggedUsers(CFCGIRequest& request)
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
        if( (diff > 180) || dtg.IsDown() ){  // skew of 180 seconds
            status = "down";
        }

        // write response
        request.OutStream.PutStr(status); // node status
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(dtg.GetNodeName()); // node name
        if( dtg.GetLocalLoginName() != NULL ){
            request.OutStream.PutChar(';');
            request.OutStream.PutStr(dtg.GetLocalUserName()); // full user name - optional
            request.OutStream.PutChar(';');
            request.OutStream.PutStr(dtg.GetLocalLoginName()); // login name - optional
        } else {
            if( (dtg.NumOfLocalUsers > 0) || (dtg.NumOfVNCRemoteUsers < 0) ){
                request.OutStream.PutChar(';');
                request.OutStream.PutStr("X+VNC seats"); // full user name - optional
                request.OutStream.PutChar(';');
                request.OutStream.PutStr(CSmallString(dtg.NumOfLocalUsers+dtg.NumOfVNCRemoteUsers)); // login name - optional
            }

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

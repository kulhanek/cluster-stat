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

bool CFCGIStatServer::_Debug(CFCGIRequest& request)
{
    // write response
    stringstream str;
    str << "<html><body>" << endl;

    NodesMutex.Lock();

    std::map<std::string,CCompNodePtr>::iterator it = Nodes.begin();
    std::map<std::string,CCompNodePtr>::iterator ie = Nodes.end();

    while( it != ie ){
        CStatDatagram dtg = it->second->Basic;

        // check node status
        CSmallString status = "up";
        // check timestamp from user stat file
        CSmallTimeAndDate stime(dtg.GetTimeStamp());
        CSmallTimeAndDate ctime;
        ctime.GetActualTimeAndDate();
        CSmallTime diff = ctime - stime;
        if( diff > 300 ){  // skew of 300 seconds
            status = "down";
        }      
        str << "<h1>" << dtg.GetNodeName() << "</h1>" << endl;
        str << "<p>Status: " << status << "</p>" << endl;
        str << "<p>Number of local sessions : " << dtg.NumOfLocalUsers << "</p>" << endl;
        str << "<ol>" << endl;
        for(int i=0; i < MAX_TTYS; i++){
            str << "<li>" << dtg.GetLocalLoginName(i) << " (" << dtg.GetLocalUserName(i) << ")</li>" << endl;
        }
        str << "</ol>" << endl;
        str << "<p>Number of remote sessions: " << dtg.NumOfRemoteUsers << "</p>" << endl;
        str << "<p>Number of VNC sessions   : " << dtg.NumOfVNCRemoteUsers << "</p>" << endl;
        str << "<ol>" << endl;
        for(int i=0; i < MAX_TTYS; i++){
            str << "<li>" << dtg.GetRemoteLoginName(i) << " (" << dtg.GetRemoteUserName(i) << ")</li>" << endl;
        }
        str << "</ol>" << endl;
        it++;
    }

    NodesMutex.Unlock();

    str << "</body></html>" << endl;

    request.OutStream.PutStr(str.str());

    // finalize request
    request.FinishRequest();

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

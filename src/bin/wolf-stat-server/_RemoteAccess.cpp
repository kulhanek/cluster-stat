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

bool CFCGIStatServer::_RemoteAccess(CFCGIRequest& request)
{
    CSmallString node;

    node = request.Params.GetValue("wakeonlan");
    if( node != NULL ){
        return(_RemoteAccessWakeOnLAN(request,node));
    }

    node = request.Params.GetValue("startvnc");
    if( node != NULL ){
        return(_RemoteAccessStartVNC(request,node));
    }

    // default is to list nodes
    return(_RemoteAccessList(request));
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessWakeOnLAN(CFCGIRequest& request,const CSmallString& node)
{
// check if node is only name of the node
    for(int i=0; i < (int)node.GetLength(); i++){
        if( isalnum(node[i]) == 0 ) {
            request.FinishRequest();
            return(false);
        }
    }

// call wolf-poweon script
    CSmallString cmd;
    cmd = "/opt/wolf-poweron/wolf-poweron --nowait \"";
    cmd << node << "\"";
    system(cmd);

// mark the node

// send the node list
    _RemoteAccessList(request);
    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessStartVNC(CFCGIRequest& request,const CSmallString& node)
{
    request.FinishRequest();
    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessList(CFCGIRequest& request)
{
    NodesMutex.Lock();

    std::map<std::string,CCompNode>::iterator it = Nodes.begin();
    std::map<std::string,CCompNode>::iterator ie = Nodes.end();

    while( it != ie ){
        CCompNode node = it->second;

        // check node status
        CSmallString status = "up";
        // check timestamp from user stat file
        CSmallTimeAndDate stime(node.Basic.GetTimeStamp());
        CSmallTimeAndDate ctime;
        ctime.GetActualTimeAndDate();
        CSmallTime diff = ctime - stime;
        if(  (diff > 180) || node.Basic.IsDown() ){  // skew of 180 seconds
            status = "down";
        }
        if( node.InPowerOnMode ){
            status = "poweron";
        }

        // write response
        request.OutStream.PutStr(status); // node status
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

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
    CSmallString user;
    user = request.Params.GetValue("REMOTE_USER");

// check if node is only name of the node
    for(int i=0; i < (int)node.GetLength(); i++){
        if( isalnum(node[i]) == 0 ) {
            CSmallString error;
            error << "illegal node name (" << node << ") from USER (" << user << ")";
            ES_ERROR(error);
            request.FinishRequest();
            return(false);
        }
    }

// call wolf-poweon script
    CSmallString cmd;
    cmd = "/opt/wolf-poweron/wolf-poweron --nowait \"";
    cmd << node << "\"";
    cout << "> User: " << user << endl;
    system(cmd);

// mark the node
    NodesMutex.Lock();

    if( Nodes.count(string(node)) == 1 ){
        Nodes[string(node)].InPowerOnMode = true;
        CSmallTimeAndDate time;
        time.GetActualTimeAndDate();
        Nodes[string(node)].PowerOnTime = time.GetSecondsFromBeginning();
    } else {
//        CCompNode node;
//        node.Basic.
    }

    NodesMutex.Unlock();

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
        CSmallTimeAndDate ctime;
        ctime.GetActualTimeAndDate();
        int diff = ctime.GetSecondsFromBeginning() - node.Basic.GetTimeStamp();
        if(  (diff > 180) || node.Basic.IsDown() ){  // skew of 180 seconds
            status = "down";
        }

        diff = ctime.GetSecondsFromBeginning() - node.PowerOnTime;
        if( node.InPowerOnMode ){
            if( diff < 300 ){
                status = "poweron";
            } else {
                status = "down";
                Nodes[string(node.Basic.GetShortNodeName())].InPowerOnMode = false;
            }
        }

        // write response
        request.OutStream.PutStr(status); // node status
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(node.Basic.GetShortNodeName()); // node name
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

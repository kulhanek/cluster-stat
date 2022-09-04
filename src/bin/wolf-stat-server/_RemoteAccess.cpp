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
#include <FileSystem.hpp>
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
//    request.Params.PrintParams();

    CSmallString ruser;
    ruser = request.Params.GetValue("REMOTE_USER");

// check if node is only name of the node
    for(int i=0; i < (int)node.GetLength(); i++){
        if( isalnum(node[i]) == 0 ) {
            CSmallString error;
            error << "illegal node name (" << node << ") from USER (" << ruser << ")";
            ES_ERROR(error);
            request.FinishRequest();
            return(false);
        }
    }

// call wolf-poweon script
    CSmallString cmd;
    cmd = "/opt/wolf-poweron/wolf-poweron --nowait --noheader \"";
    cmd << node << "\"";
    cout << "> User: " << ruser << endl;
    system(cmd);

// mark the node
    NodesMutex.Lock();

    CSmallTimeAndDate ctime;
    ctime.GetActualTimeAndDate();

    if( Nodes.count(string(node)) == 1 ){
        Nodes[string(node)].InPowerOnMode = true;
        Nodes[string(node)].PowerOnTime = ctime.GetSecondsFromBeginning();
    } else {
        CCompNode data;
        data.Basic.SetShortNodeName(node);
        data.InPowerOnMode = true;
        data.PowerOnTime  = ctime.GetSecondsFromBeginning();
        Nodes[string(node)] = data;
    }

    NodesMutex.Unlock();

// send the node list
    _RemoteAccessList(request);
    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessStartVNC(CFCGIRequest& request,const CSmallString& node)
{
    CSmallString ruser = request.Params.GetValue("REMOTE_USER");

// start VNC
    cout << "start VNC: " << ruser << "@" << node << endl;

    CSmallTimeAndDate ctime;
    ctime.GetActualTimeAndDate();

// mark the node
    NodesMutex.Lock();

    if( Nodes.count(string(node)) == 1 ){
        Nodes[string(node)].InStartVNCMode = true;
        Nodes[string(node)].StartVNCTime   = ctime.GetSecondsFromBeginning();
    } else {
        CCompNode data;
        data.Basic.SetShortNodeName(node);
        data.InStartVNCMode = true;
        data.StartVNCTime   = ctime.GetSecondsFromBeginning();
        Nodes[string(node)] = data;
    }

    NodesMutex.Unlock();

// send the node list
    _RemoteAccessList(request);
    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessList(CFCGIRequest& request)
{
    CSmallString ruser = request.Params.GetValue("REMOTE_USER");

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

        diff = ctime.GetSecondsFromBeginning() - node.StartVNCTime;
        if( node.InStartVNCMode ){
            if( diff < 300 ){
                status = "startvnc";
            } else {
                Nodes[string(node.Basic.GetShortNodeName())].InStartVNCMode = false;
            }
        }

        bool occupy = false;
        if( node.Basic.NumOfLocalUsers > 0 ){
            occupy = true;
        }
        for(int i=0; i < node.Basic.NumOfRemoteUsers; i++){
            if( node.Basic.GetRemoteLoginType(i) == 'V' ){
                occupy = true;
            }
        }
        if( occupy ){
            status = "occ";
            Nodes[string(node.Basic.GetShortNodeName())].InStartVNCMode = false;
        }

        CFileName socket = RDSKPath / ruser / node.Basic.GetNodeName();
        if( DomainName != NULL ){
            socket = socket + "." + DomainName;
        }
        CSmallString rdsk_url = "";
        if( CFileSystem::IsSocket(socket) ){
            status = "vnc";
            rdsk_url << "https://wolf.ncbr.muni.cz/bluezone/noVNC/vnc.html?host=wolf.ncbr.muni.cz&encrypt=1&path=/bluezone/rdsk/";
            rdsk_url << ruser << "/";
            rdsk_url << node.Basic.GetNodeName();
            if( DomainName != NULL ){
                rdsk_url << "." << DomainName;
            }
            rdsk_url << "&resize=remote&autoconnect=true";
            Nodes[string(node.Basic.GetShortNodeName())].InStartVNCMode = false;
        }

        // write response
        request.OutStream.PutStr(status); // node status
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(node.Basic.GetShortNodeName()); // node name
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(rdsk_url);
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

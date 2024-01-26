// =============================================================================
//  Cluster Stat Server
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
#include <sstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/format.hpp>

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
    CSmallString ruser;
    ruser = request.Params.GetValue("REMOTE_USER");
    if( ruser == NULL ){
        ruser = "NoUser";
    }

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

// final check
    if( CanPowerUp(node) == false ){
        return(_RemoteAccessList(request));
    }

    CSmallTimeAndDate ctime;
    ctime.GetActualTimeAndDate();

// mark the node
    NodesMutex.Lock();

    CCompNodePtr cnode;

    if( Nodes.count(string(node)) == 1 ){
        cnode = Nodes[string(node)];
    } else {
        cnode = CCompNodePtr(new CCompNode);
        cnode->Basic.SetNodeName(node);
        Nodes[string(node)] = cnode;
    }

    cnode->InPowerOnMode = true;
    cnode->PowerOnTime = ctime.GetSecondsFromBeginning();

    NodesMutex.Unlock();

// call poweron script
    CSmallString cmd;
    stringstream str;
    try {
        str << format(PowerOnCMD)%node;
        cmd << str.str();
    } catch (...) {
        CSmallString err;
        err << "unalble to format startrdsk command '" << StartRDSKCMD << "'";
        ES_ERROR(err);;
        cmd = NULL;
        cmd << "/bin/false";
    }

    cout << "> User: " << ruser << endl;
    int ret = system(cmd);

    if( ret != 0 ){
        CSmallString err;
        err << "unable to execute power on command '" << cmd << "'";
        ES_ERROR(cmd);
        // unable to run - do not mark the node
        cnode->InPowerOnMode = false;
    }

// send the node list
    return(_RemoteAccessList(request));
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::CanPowerUp(const CSmallString& node)
{
    EPowerStat status = Nodes[string(node)]->PowerStat;

    if( status == EPS_DOWN ){
        // we can turn on the node, which is down
        return(true);
    }

    // up
    // maintenance
    // unknown
    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessStartVNC(CFCGIRequest& request,const CSmallString& node)
{
    CSmallString ruser      = request.Params.GetValue("REMOTE_USER");
    if( ruser == NULL ){
        ruser = "NoUser";
    }

    CSmallString krb5ccname = request.Params.GetValue("KRB5CCNAME");
    if( krb5ccname == NULL ){
        krb5ccname = "NONE";
    }

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

// final check
    if( CanStartRDSK(node) == false ){
        return(_RemoteAccessList(request));
    }

    CSmallTimeAndDate ctime;
    ctime.GetActualTimeAndDate();

// mark the node
    NodesMutex.Lock();

    CCompNodePtr cnode;

    if( Nodes.count(string(node)) == 1 ){
        cnode = Nodes[string(node)];
    } else {
        cnode = CCompNodePtr(new CCompNode);
        cnode->Basic.SetNodeName(node);
        Nodes[string(node)] = cnode;
    }

    cnode->InStartVNCMode = true;
    cnode->StartVNCTime   = ctime.GetSecondsFromBeginning();

    NodesMutex.Unlock();

// start RDSK
    CSmallString cmd;
    stringstream str;
    try{
        str << format(StartRDSKCMD)%krb5ccname%ruser%node;
        cmd << str.str();
    } catch (...) {
        CSmallString err;
        err << "unable to format startrdsk command '" << StartRDSKCMD << "'";
        ES_ERROR(err);
        cmd = NULL;
        cmd << "/bin/false";
    }

    cout << "> Start RDSK: " << ruser << "@" << node << endl;
    int ret = system(cmd);

    if( ret != 0 ){
        CSmallString err;
        err << "unable to execute start rdks command '" << cmd << "'";
        ES_ERROR(cmd);
        // unable to run - do not mark the node
        cnode->InStartVNCMode = false;
    }

// send the node list
    return(_RemoteAccessList(request));
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::CanStartRDSK(const CSmallString& node)
{
    EPowerStat status = Nodes[string(node)]->PowerStat;

    if( status == EPS_UP ){
        // we can start RDSK on node, which is UP
        return(true);
    }

    // down
    // maintenance
    // unknown
    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::_RemoteAccessList(CFCGIRequest& request)
{
    CSmallString ruser = request.Params.GetValue("REMOTE_USER");

    NodesMutex.Lock();

    std::map<std::string,CCompNodePtr>::iterator it = Nodes.begin();
    std::map<std::string,CCompNodePtr>::iterator ie = Nodes.end();

    CSmallTimeAndDate ctime;
    ctime.GetActualTimeAndDate();

    int diff;

    while( it != ie ){
        CCompNodePtr node = it->second;

        CSmallString status = "up";
        CSmallString rdsk_url = "";

        // node status
        EPowerStat nstat = node->PowerStat;
        if( (nstat == EPS_MAINTANANCE) || (nstat == EPS_UNKNOWN) ){
            status = "maintenance";
            diff = ctime.GetSecondsFromBeginning() - node->Basic.GetTimeStamp();
            if( diff > 240 ){  // skew 4m
                node->Clear();
            }
        }

        if( status != "maintenance" ){
            if( node->InPowerOnMode ){
                diff = ctime.GetSecondsFromBeginning() - node->PowerOnTime;
                if( diff < 240 ){       // allowed skew 4m
                    status = "poweron";
                }
            }
        }

        if( (status != "maintenance") && (status != "poweron") ){
            if( node->Basic.IsDown() || (nstat == EPS_DOWN) ){
                status = "maintenance"; // keep node in maintenance during poweroff procedure
            }
            if( status == "maintenance" ) {
                diff = ctime.GetSecondsFromBeginning() - node->Basic.GetTimeStamp();
                if( diff > 240 ){   // skew 4m
                    status = "down";
                    node->Clear();
                }
            }
        }

        if( status == "up" ){
            diff = ctime.GetSecondsFromBeginning() - node->Basic.GetTimeStamp();
            if( (nstat == EPS_UP) && (diff > 180) ){
                // some weird node status
                status = "maintenance";
            }
        }

        if( (status != "poweron") && (status != "down") && (status != "maintenance") ) {
            diff = ctime.GetSecondsFromBeginning() - node->StartVNCTime;
            if( node->InStartVNCMode ){
                if( diff < 60 ){
                    status = "startvnc";
                } else {
                    node->InStartVNCMode = false;
                }
            }

            bool occupy = false;
            if( node->Basic.NumOfLocalUsers > 0 ){
                occupy = true;
            }
            for(int i=0; i < node->Basic.NumOfRemoteUsers; i++){
                if( node->Basic.GetRemoteLoginType(i) == 'R' ){
                    occupy = true;
                }
                if( node->Basic.GetRemoteLoginType(i) == 'V' ){
                    occupy = true;
                }
            }

            CFileName socket = RDSKPath / ruser / node->Basic.GetNodeName();
            if( DomainName != NULL ){
                socket = socket + "." + DomainName;
            }
            if( IsSocketLive(socket) ){
                status = "vnc";
                stringstream str;
                CSmallString rnode;
                rnode << node->Basic.GetNodeName();
                if( DomainName != NULL ){
                    rnode << "." << DomainName;
                }
                try{
                    CSmallString server = request.Params.GetValue("SERVER_NAME");
                    str << format(URLTmp)%server%ruser%rnode;
                } catch(...) {
                    ES_ERROR("wrong url tmp");
                }

                rdsk_url << str.str();

                node->InStartVNCMode = false;
            }

            if( occupy && (status != "startvnc") && (status != "vnc") ){
                status = "occ";
            }
        }

        if( (status == "up") || (status == "vnc") ){
            CSmallString quota;
            stringstream str;
            try{
                str << format(QuotaFlag)%ruser;
                quota << str.str();
            } catch (...) {
                CSmallString err;
                err << "unable to format quota flag '" << QuotaFlag << "'";
                ES_ERROR(err);
                quota = NULL;
                quota << "blabla";
            }
            if( CFileSystem::IsFile(quota) == true ){
                status = "quota";
            }
        }

        // write response
        request.OutStream.PutStr(status); // node status
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(node->Basic.GetNodeName()); // node name
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

//------------------------------------------------------------------------------

bool CFCGIStatServer::IsSocketLive(const CSmallString& socket)
{
    // is it socket?
    if( CFileSystem::IsSocket(socket) == false ) return(false);

    bool isactive = false;
    CSmallString cmd;
    cmd << "/usr/bin/cat /proc/net/unix";
    FILE* p_sf = popen(cmd,"r");
    if( p_sf ){
        CSmallString buffer;
        while( buffer.ReadLineFromFile(p_sf,true,true) ){
            if( buffer.FindSubString(socket) != -1 ) isactive = true;
        }
        pclose(p_sf);
    }

    return(isactive);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

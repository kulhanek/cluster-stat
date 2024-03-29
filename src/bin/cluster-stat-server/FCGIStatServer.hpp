#ifndef FCGIStatServerH
#define FCGIStatServerH
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

#include <FCGIServer.hpp>
#include <XMLDocument.hpp>
#include <FileName.hpp>
#include <ServerOptions.hpp>
#include <StatDatagram.hpp>
#include <VerboseStr.hpp>
#include <TerminalStr.hpp>
#include <ServerWatcher.hpp>
#include <SimpleMutex.hpp>
#include <TemplateParams.hpp>
#include <StatServer.hpp>
#include <SmallTimeAndDate.hpp>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <BatchSystemWatcher.hpp>

//------------------------------------------------------------------------------

enum EPowerStat {
    EPS_DOWN,
    EPS_UP,
    EPS_MAINTANANCE,
    EPS_UNKNOWN,
};

//------------------------------------------------------------------------------

class CCompNode {
public:
    CCompNode(void);
    void Clear(void);
public:
    CStatDatagram   Basic;
    bool            InPowerOnMode;
    int             PowerOnTime;

    bool            InStartVNCMode;
    int             StartVNCTime;

    EPowerStat      PowerStat;
    int             NCPUs;
    int             NGPUs;
};

typedef boost::shared_ptr<CCompNode>   CCompNodePtr;

//------------------------------------------------------------------------------

class CFCGIStatServer : public CFCGIServer {
public:
    CFCGIStatServer(void);

// main methods ----------------------------------------------------------------
    /// init options
    int Init(int argc,char* argv[]);

    /// main part of program
    bool Run(void);

    /// finalize
    void Finalize(void);

    /// register node
    void RegisterNode(CStatDatagram& dtg);


    /// update node power status
    void UpdateNodePowerStatus(struct batch_status* p_node_attrs);

// section of private data -----------------------------------------------------
private:
    CServerOptions      Options;
    CXMLDocument        ServerConfig;
    CTerminalStr        Console;
    CVerboseStr         vout;
    CServerWatcher      Watcher;
    CBatchSystemWatcher BatchSystem;
    CStatServer         StatServer;
    CSimpleMutex        NodesMutex;
    int                 FCGIPort;
    int                 StatPort;
    unsigned int        MaxNodes;
    CFileName           RDSKPath;
    CFileName           DomainName;
    CSmallString        URLTmp;
    CSmallString        PowerOnCMD;
    CSmallString        StartRDSKCMD;
    CSmallString        QuotaFlag;

    std::map<std::string,CCompNodePtr> Nodes;

    static  void CtrlCSignalHandler(int signal);
    virtual bool AcceptRequest(void);

    // web pages handlers ------------------------------------------------------
    bool _Error(CFCGIRequest& request);
    bool _ListLoggedUsers(CFCGIRequest& request);
    bool _ListAllSeats(CFCGIRequest& request);
    bool _RemoteAccess(CFCGIRequest& request);
    bool _RemoteAccessWakeOnLAN(CFCGIRequest& request,const CSmallString& node);
    bool _RemoteAccessStartVNC(CFCGIRequest& request,const CSmallString& node);
    bool _RemoteAccessList(CFCGIRequest& request);
    bool _Debug(CFCGIRequest& request);

    bool ProcessCommonParams(CFCGIRequest& request,
                             CTemplateParams& template_params);

    bool HasKerberos(CFCGIRequest& request);
    bool IsSocketLive(const CSmallString& socket);
    bool CanPowerUp(const CSmallString& node);
    bool CanStartRDSK(const CSmallString& node);

    // configuration options ---------------------------------------------------
    bool LoadConfig(void);
};

//------------------------------------------------------------------------------

extern CFCGIStatServer ClusterStatServer;

//------------------------------------------------------------------------------

#endif

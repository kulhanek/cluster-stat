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

#include <FCGIStatServer.hpp>
#include <FCGIRequest.hpp>
#include <ErrorSystem.hpp>
#include <SmallTimeAndDate.hpp>
#include <signal.h>
#include <XMLElement.hpp>
#include <XMLParser.hpp>
#include <TemplatePreprocessor.hpp>
#include <TemplateCache.hpp>
#include <Template.hpp>
#include <XMLPrinter.hpp>
#include <XMLText.hpp>
#include <iostream>
#include <pbs_ifl.h>
#include <PBSProAttr.hpp>

//------------------------------------------------------------------------------

using namespace std;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CFCGIStatServer ClusterStatServer;

MAIN_ENTRY_OBJECT(ClusterStatServer)

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CCompNode::CCompNode(void)
{
    InPowerOnMode = false;
    PowerOnTime = 0;

    InStartVNCMode = false;
    StartVNCTime = 0;

    PowerStat = EPS_UNKNOWN;
    NGPUs = 0;

    Basic.Clear();
}

//------------------------------------------------------------------------------

void CCompNode::Clear(void)
{
    InPowerOnMode = false;
    PowerOnTime = 0;

    InStartVNCMode = false;
    StartVNCTime = 0;

    CSmallString node = Basic.GetNodeName();
    Basic.Clear();
    Basic.SetNodeName(node);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CFCGIStatServer::CFCGIStatServer(void)
{
    FCGIPort        = 32597;
    StatPort        = 32598;
    MaxNodes        = 100;
    RDSKPath        = "/var/lib/websockify";
    DomainName      = "ncbr.muni.cz";
    URLTmp          = "https://%1%/bluezone/noVNC/vnc.html?path=/bluezone/rdsk/%2%/%3%&autoconnect=true";
    PowerOnCMD      = "/opt/node-poweron/node-poweron --nowait --noheader \"%1%\"";
    StartRDSKCMD    = "/opt/node-stat-server/startrdsk \"%1%\" \"%2%\" \"%3%\"";
    QuotaFlag       = "/home/%1%.overquota";
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

int CFCGIStatServer::Init(int argc,char* argv[])
{
    // encode program options, all check procedures are done inside of CABFIntOpts
    int result = Options.ParseCmdLine(argc,argv);

    // should we exit or was it error?
    if( result != SO_CONTINUE ) return(result);

    // attach verbose stream to terminal stream and set desired verbosity level
    vout.Attach(Console);
    if( Options.GetOptVerbose() ) {
        vout.Verbosity(CVerboseStr::high);
    } else {
        vout.Verbosity(CVerboseStr::low);
    }

    CSmallTimeAndDate dt;
    dt.GetActualTimeAndDate();

    vout << low;
    vout << endl;
    vout << "# ==============================================================================" << endl;
    vout << "# cluster-stat-server.fcgi started at " << dt.GetSDateAndTime() << endl;
    vout << "# ==============================================================================" << endl;

    // load server config
    if( LoadConfig() == false ) return(SO_USER_ERROR);

    return(SO_CONTINUE);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CFCGIStatServer::Run(void)
{
    // CtrlC signal
    signal(SIGINT,CtrlCSignalHandler);
    signal(SIGTERM,CtrlCSignalHandler);

    SetPort(FCGIPort);
    StatServer.SetPort(StatPort);

    // start servers
    Watcher.StartThread();          // watcher
    BatchSystem.StartThread();      // batch system
    StatServer.StartThread();       // stat server
    if( StartServer() == false ) {  // fcgi server
        return(false);
    }

    vout << low;
    vout << "Waiting for server terminations ..." << endl;
    WaitForServer();

    vout << "Waiting for STAT server termination ..." << endl;
    StatServer.TerminateServer();
    StatServer.WaitForThread();

    vout << "Waiting for watcher server termination ..." << endl;
    Watcher.TerminateThread();
    Watcher.WaitForThread();

    vout << "Waiting for batch system server termination ..." << endl;
    BatchSystem.TerminateThread();
    BatchSystem.WaitForThread();

    vout << "# Number of client total requests      = " << StatServer.AllRequests << endl;
    vout << "# Number of client successful requests = " << StatServer.SuccessfulRequests << endl;
    vout << "# Number of nodes                      = " << Nodes.size() << endl;
    vout << endl;

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CFCGIStatServer::Finalize(void)
{
    CSmallTimeAndDate dt;
    dt.GetActualTimeAndDate();

    vout << low;
    vout << "# ==============================================================================" << endl;
    vout << "# cluster-stat-server.fcgi terminated at " << dt.GetSDateAndTime() << endl;
    vout << "# ==============================================================================" << endl;

    if( ErrorSystem.IsError() || Options.GetOptVerbose() ){
        vout << low;
        ErrorSystem.PrintErrors(vout);
    }

    vout << endl;
}

//------------------------------------------------------------------------------

void CFCGIStatServer::RegisterNode(CStatDatagram& dtg)
{
    NodesMutex.Lock();

    string node = string(dtg.GetNodeName());

    if( MaxNodes == Nodes.size() ){
        // too many nodes and the node is not registered yet
        if( Nodes.count(node) != 1 ){
            NodesMutex.Unlock();
            ES_ERROR("too many nodes - skiping new registration");
            return;
        }
    }

    if( Nodes.count(node) == 1 ) {
        // the node is registered
        Nodes[node]->Basic = dtg;

        // clear power on status
        Nodes[node]->InPowerOnMode  = false;
        Nodes[node]->PowerOnTime    = 0;

    } else {
        // new registration
        CCompNodePtr data(new CCompNode);
        data->Basic          = dtg;
        data->InPowerOnMode  = false;
        data->PowerOnTime    = 0;

        Nodes[node] = data;
    }

    NodesMutex.Unlock();
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CFCGIStatServer::AcceptRequest(void)
{
    // this is simple FCGI Application with 'Hello world!'

    CFCGIRequest request;

    // accept request
    if( request.AcceptRequest(this) == false ) {
        ES_ERROR("unable to accept request");
        // unable to accept request
        return(false);
    }

    request.Params.LoadParamsFromQuery();

    // write document
    request.OutStream.PutStr("Content-type: text/html\r\n");
    request.OutStream.PutStr("\r\n");

    // get request id
    CSmallString action;
    action = request.Params.GetValue("action");

    bool result = false;

    // user stat
    if( (action == NULL) || (action == "loggedusers") ) {
        result = _ListLoggedUsers(request);
    }
    if( action == "allseats" ) {
        result = _ListAllSeats(request);
    }
    if( action == "remote" ) {
        result = _RemoteAccess(request);
    }
    if( action == "debug" ) {
        result = _Debug(request);
    }

    // error handle -----------------------
    if( result == false ) {
        ES_ERROR("error");
        result = _Error(request);
    }
    if( result == false ) request.FinishRequest(); // at least try to finish request

    return(true);
}

//------------------------------------------------------------------------------

bool CFCGIStatServer::ProcessCommonParams(CFCGIRequest& request,
        CTemplateParams& template_params)
{
    CSmallString server_script_uri;

    if( request.Params.GetValue("SERVER_PORT") == "443" ) {
        server_script_uri = "https://";
    } else {
        server_script_uri = "http://";
    }

    server_script_uri << request.Params.GetValue("SERVER_NAME");
    if( (server_script_uri.GetLength() > 0) && (server_script_uri[server_script_uri.GetLength()-1] != '/') ){
        server_script_uri << "/";
    }
    server_script_uri << request.Params.GetValue("SCRIPT_NAME");

    // FCGI setup
    template_params.SetParam("SERVERSCRIPTURI",server_script_uri);
    template_params.SetParam("CLUSTERSTAT_VER",StatBuildVersion);

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CFCGIStatServer::CtrlCSignalHandler(int signal)
{
    ClusterStatServer.vout << endl << endl;
    ClusterStatServer.vout << "SIGINT or SIGTERM signal recieved. Initiating server shutdown!" << endl;
    ClusterStatServer.vout << "Waiting for server finalization ... " << endl;
    ClusterStatServer.TerminateServer();
    if( ! ClusterStatServer.Options.GetOptVerbose() ) ClusterStatServer.vout << endl;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CFCGIStatServer::LoadConfig(void)
{
    CFileName    config_path;

    // FIXME
    config_path = "/opt/node-stat-server/3.0/etc";
    config_path = config_path / "cluster-stat-server.xml";

    CXMLParser xml_parser;
    xml_parser.SetOutputXMLNode(&ServerConfig);

    if( xml_parser.Parse(config_path) == false ) {
        CSmallString error;
        error << "unable to load server config";
        ES_ERROR(error);
        return(false);
    }

    CXMLElement* p_ele = ServerConfig.GetChildElementByPath("config/servers");
    if( p_ele != NULL ) {
        // optional setup
        p_ele->GetAttribute("fcgiport",FCGIPort);
        p_ele->GetAttribute("statport",StatPort);
        p_ele->GetAttribute("maxnodes",MaxNodes);
        p_ele->GetAttribute("rdskpath",RDSKPath);
        p_ele->GetAttribute("domain",DomainName);
        p_ele->GetAttribute("url",URLTmp);
        p_ele->GetAttribute("poweron",PowerOnCMD);
        p_ele->GetAttribute("rdsk",StartRDSKCMD);
        p_ele->GetAttribute("quota",QuotaFlag);
    }

    vout << "#" << endl;
    vout << "# === [servers] ================================================================" << endl;
    vout << "# FCGI Port (fcgiport)     = " << FCGIPort << endl;
    vout << "# Stat Port (statport)     = " << StatPort << endl;
    vout << "# Max nodes (maxnodes)     = " << MaxNodes << endl;
    vout << "# RDSK Path (rdskpath)     = " << RDSKPath << endl;
    vout << "# Domain name (domain)     = " << DomainName << endl;
    vout << "# URL Template (url)       = " << URLTmp << endl;
    vout << "# PowerOn CMD (poweron)    = " << PowerOnCMD << endl;
    vout << "# Start RDSK (rdsk)        = " << StartRDSKCMD << endl;
    vout << "# Quota overdue (quota)    = " << QuotaFlag << endl;

    vout << "#" << endl;
    vout << "# === [nodes] ==================================================================" << endl;
    CXMLElement* p_nodes = ServerConfig.GetChildElementByPath("config/nodes");
    if( p_nodes != NULL ) {
        CXMLElement* p_node = p_nodes->GetFirstChildElement("node");
        while( p_node != NULL ) {
            CSmallString name;
            if( p_node->GetAttribute("name",name) == true ){
                vout << "  * " << name << endl;
                CCompNodePtr node(new CCompNode);
                node->Basic.SetNodeName(name);
                Nodes[string(name)]=node;
            }
            p_node = p_node->GetNextSiblingElement("node");
        }
    }

    CXMLElement* p_watcher = ServerConfig.GetChildElementByPath("config/watcher");
    if( Watcher.ProcessWatcherControl(vout,p_watcher) == false ) return(false);

    CXMLElement* p_batchsys = ServerConfig.GetChildElementByPath("config/batch_system");
     if( BatchSystem.ProcessBatchSystemControl(vout,p_batchsys) == false ) return(false);
    return(true);
}

//------------------------------------------------------------------------------

void CFCGIStatServer::UpdateNodePowerStatus(struct batch_status* p_node_attrs)
{
    NodesMutex.Lock();

    ofstream ofs("/tmp/node-stat-server-pbs.log");

    while( p_node_attrs != NULL ){
        string node_name = string(p_node_attrs->name);
        // get short name
        node_name = node_name.substr(0,node_name.find("."));
        ofs << "node: " << node_name <<  endl;
        if( Nodes.count(node_name) == 1 ){
            CCompNodePtr node = Nodes[node_name];
            get_attribute(p_node_attrs->attribs,"resources_available","ncpus",node->NCPUs);
            get_attribute(p_node_attrs->attribs,"resources_available","ngpus",node->NGPUs);
            CSmallString ps;
            get_attribute(p_node_attrs->attribs,"resources_available","power_status",ps);
            EPowerStat status = EPS_UNKNOWN;
            if( ps == "maintenance" ) status = EPS_MAINTANANCE;
            if( ps == "up" ) status = EPS_UP;
            if( ps == "down" ) status = EPS_DOWN;
            ofs << "node: " << node_name << " st:" << status <<" (" << ps <<")" << endl;
            node->PowerStat = status;
        }

        p_node_attrs = p_node_attrs->next;
    }

    ofs.close();

    NodesMutex.Unlock();
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

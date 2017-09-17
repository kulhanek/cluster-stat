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

#include "WolfStatServer.hpp"
#include <FCGIRequest.hpp>
#include <ErrorSystem.hpp>
#include <SmallTimeAndDate.hpp>
#include <signal.h>
#include <XMLElement.hpp>
#include <XMLParser.hpp>
#include "prefix.h"
#include <TemplatePreprocessor.hpp>
#include <TemplateCache.hpp>
#include <Template.hpp>
#include <XMLPrinter.hpp>
#include <XMLText.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace boost;

//------------------------------------------------------------------------------

const char* LibBuildVersion_WOLF_Web = "wolf-stat-server 2.0";

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CWolfStatServer WolfStatServer;

MAIN_ENTRY_OBJECT(WolfStatServer)

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CWolfStatServer::CWolfStatServer(void)
{
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

int CWolfStatServer::Init(int argc,char* argv[])
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
    vout << "# wolf-stat-server.fcgi started at " << dt.GetSDateAndTime() << endl;
    vout << "# ==============================================================================" << endl;

    // load server config
    if( LoadConfig() == false ) return(SO_USER_ERROR);

    return(SO_CONTINUE);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CWolfStatServer::Run(void)
{
    // CtrlC signal
    signal(SIGINT,CtrlCSignalHandler);
    signal(SIGTERM,CtrlCSignalHandler);

    SetPort(GetPortNumber());

    // start servers
    Watcher.StartThread(); // watcher
    if( StartServer() == false ) { // and fcgi server
        return(false);
    }

    vout << low;
    vout << "Waiting for server termination ..." << endl;
    WaitForServer();

    Watcher.TerminateThread();
    Watcher.WaitForThread();

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CWolfStatServer::Finalize(void)
{
    CSmallTimeAndDate dt;
    dt.GetActualTimeAndDate();

    vout << low;
    vout << "# ==============================================================================" << endl;
    vout << "# wolf-stat-server.fcgi terminated at " << dt.GetSDateAndTime() << endl;
    vout << "# ==============================================================================" << endl;

    if( ErrorSystem.IsError() || Options.GetOptVerbose() ){
        vout << low;
        ErrorSystem.PrintErrors(vout);
    }

    vout << endl;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CWolfStatServer::AcceptRequest(void)
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

    // error handle -----------------------
    if( result == false ) {
        ES_ERROR("error");
        result = _Error(request);
    }
    if( result == false ) request.FinishRequest(); // at least try to finish request

    return(true);
}

//------------------------------------------------------------------------------

bool CWolfStatServer::ProcessTemplate(CFCGIRequest& request,
                                       const CSmallString& template_name,
                                       CTemplateParams& template_params)
{
    // template --------------------------------------------------------
    CTemplate* p_tmp = TemplateCache.OpenTemplate(template_name);

    if( p_tmp == NULL ) {
        ES_ERROR("unable to open template");
        return(false);
    }

    // preprocess template ---------------------------------------------
    CTemplatePreprocessor preprocessor;
    CXMLDocument          output_xml;

    preprocessor.SetInputTemplate(p_tmp);
    preprocessor.SetOutputDocument(&output_xml);

    if( preprocessor.PreprocessTemplate(&template_params) == false ) {
        ES_ERROR("unable to preprocess template");
        return(false);
    }

    // print output ----------------------------------------------------
    CXMLPrinter xml_printer;

    xml_printer.SetPrintedXMLNode(&output_xml);
    xml_printer.SetPrintAsItIs(true);

    unsigned char* p_data;
    unsigned int   len = 0;

    if( (p_data = xml_printer.Print(len)) == NULL ) {
        ES_ERROR("unable to print output");
        return(false);
    }

    request.OutStream.PutStr((const char*)p_data,len);

    delete[] p_data;

    request.FinishRequest();

    return(true);
}

//------------------------------------------------------------------------------

bool CWolfStatServer::ProcessCommonParams(CFCGIRequest& request,
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
    template_params.SetParam("WOLFVER",LibBuildVersion_WOLF_Web);

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CWolfStatServer::CtrlCSignalHandler(int signal)
{
    WolfStatServer.vout << endl << endl;
    WolfStatServer.vout << "SIGINT or SIGTERM signal recieved. Initiating server shutdown!" << endl;
    WolfStatServer.vout << "Waiting for server finalization ... " << endl;
    WolfStatServer.TerminateServer();
    if( ! WolfStatServer.Options.GetOptVerbose() ) WolfStatServer.vout << endl;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CWolfStatServer::LoadConfig(void)
{
    CFileName    config_path;

    config_path = ETCDIR;
    config_path = config_path / "wolf-stat-server.xml";

    CXMLParser xml_parser;
    xml_parser.SetOutputXMLNode(&ServerConfig);

    if( xml_parser.Parse(config_path) == false ) {
        CSmallString error;
        error << "unable to load server config";
        ES_ERROR(error);
        return(false);
    }

    // temaplate_dir
    CFileName temp_dir;
    temp_dir = CFileName(PREFIX) / "var" / "html" / "wolf-stat-server" / "templates";

    TemplateCache.SetTemplatePath(temp_dir);

    vout << "#" << endl;
    vout << "# === [server] =================================================================" << endl;
    vout << "# FCGI Port (port) = " << GetPortNumber() << endl;
    vout << "#" << endl;

    vout << "# === [userstat] ===============================================================" << endl;
    vout << "# Logged user stat dir (userstat/dir)       = " << GetLoggedUserStatDir() << endl;
    vout << "# Logged user stat filter (userstat/filter) = " << GetLoggedUserStatFilter() << endl;
    vout << "#" << endl;

    CXMLElement* p_watcher = ServerConfig.GetChildElementByPath("config/watcher");
    Watcher.ProcessWatcherControl(vout,p_watcher);
    vout << "#" << endl;

    vout << "# === [internal] ===============================================================" << endl;
    vout << "# Templates = " << temp_dir << endl;
    vout << "" << endl;

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

int CWolfStatServer::GetPortNumber(void)
{
    int setup = 32597;
    CXMLElement* p_ele = ServerConfig.GetChildElementByPath("config/server");
    if( p_ele == NULL ) {
        ES_ERROR("unable to open config path");
        return(setup);
    }
    if( p_ele->GetAttribute("port",setup) == false ) {
        ES_ERROR("unable to get setup item");
        return(setup);
    }
    return(setup);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

const CSmallString CWolfStatServer::GetLoggedUserStatDir(void)
{
    CSmallString setup;
    CXMLElement* p_ele = ServerConfig.GetChildElementByPath("config/userstat");
    if( p_ele == NULL ) {
        ES_ERROR("unable to open config/userstat path");
        return(setup);
    }
    if( p_ele->GetAttribute("dir",setup) == false ) {
        ES_ERROR("unable to get dir item");
        return(setup);
    }
    return(setup);
}

//------------------------------------------------------------------------------

const CSmallString CWolfStatServer::GetLoggedUserStatFilter(void)
{
    CSmallString setup;
    CXMLElement* p_ele = ServerConfig.GetChildElementByPath("config/userstat");
    if( p_ele == NULL ) {
        ES_ERROR("unable to open config/userstat path");
        return(setup);
    }
    if( p_ele->GetAttribute("filter",setup) == false ) {
        ES_ERROR("unable to get filter item");
        return(setup);
    }
    return(setup);
}

//------------------------------------------------------------------------------

const CSmallString CWolfStatServer::Transform(const CSmallString& text)
{
    // substitute # by %23
    string stext = string(text);
    boost::algorithm::replace_all(stext,"#","%23");
    return(stext);
}

//------------------------------------------------------------------------------

const CSmallString CWolfStatServer::GetShortName(const CSmallString& fqdn)
{
    string stext = string(fqdn);
    vector<string>  words;
    split(words,stext,is_any_of("."));
    if( words.size() >= 1 ) return( words[0] );
    return("");
}

//------------------------------------------------------------------------------

const CSmallString CWolfStatServer::GetTemplateName(const CSmallString& base_name)
{
    CSmallString lang = "CS";
    CSmallString full_name;
    full_name << base_name << lang << ".html";
    return(full_name);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

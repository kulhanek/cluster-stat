#ifndef WolfStatServerH
#define WolfStatServerH
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

#include <FCGIServer.hpp>
#include <XMLDocument.hpp>
#include <FileName.hpp>
#include <TemplateParams.hpp>
#include "WolfStatOptions.hpp"
#include <VerboseStr.hpp>
#include <TerminalStr.hpp>
#include "ServerWatcher.hpp"

//------------------------------------------------------------------------------

class CWolfStatServer : public CFCGIServer {
public:
    CWolfStatServer(void);

// main methods ----------------------------------------------------------------
    /// init options
    int Init(int argc,char* argv[]);

    /// main part of program
    bool Run(void);

    /// finalize
    void Finalize(void);

// section of private data -----------------------------------------------------
private:
    CWolfStatOptions    Options;
    CXMLDocument        ServerConfig;
    CTerminalStr        Console;
    CVerboseStr         vout;
    CServerWatcher      Watcher;

    static  void CtrlCSignalHandler(int signal);

    virtual bool AcceptRequest(void);

    // web pages handlers ------------------------------------------------------
    bool _Error(CFCGIRequest& request);
    bool _ListLoggedUsers(CFCGIRequest& request);

    bool ProcessCommonParams(CFCGIRequest& request,
                             CTemplateParams& template_params);

    bool ProcessTemplate(CFCGIRequest& request,
                         const CSmallString& template_name,
                         CTemplateParams& template_params);

    // configuration options ---------------------------------------------------
    bool LoadConfig(void);

    // fcgi server
    int                GetPortNumber(void);

    // statistics
    const CSmallString GetLoggedUserStatDir(void);
    const CSmallString GetLoggedUserStatFilter(void);

    // transform string with #
    const CSmallString Transform(const CSmallString& text);

    // get short name from FQDN
    const CSmallString GetShortName(const CSmallString& fqdn);

    // get template name
    const CSmallString GetTemplateName(const CSmallString& base_name);
};

//------------------------------------------------------------------------------

extern CWolfStatServer WolfStatServer;

//------------------------------------------------------------------------------

#endif

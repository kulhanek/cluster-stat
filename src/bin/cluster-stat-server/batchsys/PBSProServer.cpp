// =============================================================================
// ABS - Advanced Batch System
// -----------------------------------------------------------------------------
//    Copyright (C) 2011      Petr Kulhanek, kulhanek@chemi.muni.cz
//    Copyright (C) 2001-2008 Petr Kulhanek, kulhanek@chemi.muni.cz
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

#include "PBSProServer.hpp"
#include <ErrorSystem.hpp>
#include <iostream>
#include <pbs_ifl.h>
#include <stdlib.h>
#include <string.h>
#include <PBSProAttr.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <FileSystem.hpp>
#include <XMLElement.hpp>

//------------------------------------------------------------------------------

using namespace std;
using namespace boost;

//------------------------------------------------------------------------------

CPBSProServer PBSPro;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CPBSProServer::CPBSProServer(void)
{
    ServerID = 0;

    pbspro_connect = NULL;
    pbspro_disconnect = NULL;
    pbspro_statserver = NULL;
    pbspro_statque = NULL;
    pbspro_stathost = NULL;
    pbspro_statvnode = NULL;
    pbspro_statjob = NULL;
    pbspro_selstat = NULL;
    pbspro_statfree = NULL;
    pbspro_geterrmsg = NULL;
    pbspro_strerror = NULL;
    pbspro_locjob = NULL;
    pbspro_errno = NULL;
}

//------------------------------------------------------------------------------

CPBSProServer::~CPBSProServer(void)
{
    DisconnectFromServer();
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CPBSProServer::Init(const CSmallString& libs_tok,const CSmallString& server_name)
{
    // find suitable library
    vector<string> libs;
    string         slibs_tok(libs_tok);
    split(libs,slibs_tok,is_any_of(":"),boost::token_compress_on);
    vector<string>::iterator it = libs.begin();
    vector<string>::iterator ie = libs.end();

    while( it != ie ){
        CSmallString lib(*it);
        if( CFileSystem::IsFile(lib) == true ){
            PBSProLibName = lib;
            break;
        }
        it++;
    }

    // record found path to libpbs.so for debugging
    CSmallString w;
    w << "libpbs.so: " << PBSProLibName;
    ES_WARNING(w);

    if( InitSymbols() == false ){
        ES_ERROR("unable to init symbols");
        return(false);
    }

    ServerName = server_name;

    if( ConnectToServer() == false ){
        ES_TRACE_ERROR("unable to connect to server");
        return(false);
    }

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CPBSProServer::InitSymbols(void)
{
    if( PBSProLib.Open(PBSProLibName) == false ){
        ES_ERROR("unable to load batch system library");
        return(false);
    }

    // load symbols
    bool status = true;
    pbspro_connect     = (PBS_CONNECT)PBSProLib.GetProcAddress("pbs_connect");
    if( pbspro_connect == NULL ){
        ES_ERROR("unable to bind to pbs_connect");
        status = false;
    }
    pbspro_disconnect  = (PBS_DISCONNECT)PBSProLib.GetProcAddress("pbs_disconnect");
    if( pbspro_disconnect == NULL ){
        ES_ERROR("unable to bind to pbs_disconnect");
        status = false;
    }
    pbspro_statserver  = (PBS_STATSERVER)PBSProLib.GetProcAddress("pbs_statserver");
    if( pbspro_statserver == NULL ){
        ES_ERROR("unable to bind to pbs_statserver");
        status = false;
    }
    pbspro_statque  = (PBS_STATQUE)PBSProLib.GetProcAddress("pbs_statque");
    if( pbspro_statque == NULL ){
        ES_ERROR("unable to bind to pbs_statque");
        status = false;
    }
    pbspro_stathost  = (PBS_STATHOST)PBSProLib.GetProcAddress("pbs_stathost");
    if( pbspro_stathost == NULL ){
        ES_ERROR("unable to bind to pbs_stathost");
        status = false;
    }
    pbspro_statvnode  = (PBS_STATVNODE)PBSProLib.GetProcAddress("pbs_statvnode");
    if( pbspro_statvnode == NULL ){
        ES_ERROR("unable to bind to pbs_statvnode");
        status = false;
    }
    pbspro_statjob  = (PBS_STATJOB)PBSProLib.GetProcAddress("pbs_statjob");
    if( pbspro_statjob == NULL ){
        ES_ERROR("unable to bind to pbs_statjob");
        status = false;
    }
    pbspro_selstat  = (PBS_SELSTATJOB)PBSProLib.GetProcAddress("pbs_selstat");
    if( pbspro_selstat == NULL ){
        ES_ERROR("unable to bind to pbs_selstat");
        status = false;
    }
    pbspro_statfree  = (PBS_STATFREE)PBSProLib.GetProcAddress("pbs_statfree");
    if( pbspro_statfree == NULL ){
        ES_ERROR("unable to bind to pbs_statfree");
        status = false;
    }
    pbspro_geterrmsg  = (PBS_GETERRMSG)PBSProLib.GetProcAddress("pbs_geterrmsg");
    if( pbspro_geterrmsg == NULL ){
        ES_ERROR("unable to bind to pbs_geterrmsg");
        status = false;
    }
    pbspro_locjob  = (PBS_LOCJOB)PBSProLib.GetProcAddress("pbs_locjob");
    if( pbspro_locjob == NULL ){
        ES_ERROR("unable to bind to pbspro_locjob");
        status = false;
    }
    pbspro_errno  = (PBS_ERRNO)PBSProLib.GetProcAddress("__pbs_errno_location");
    if( pbspro_errno == NULL ){
        ES_ERROR("unable to bind to __pbs_errno_location");
        status = false;
    }
    return(status);
}

//------------------------------------------------------------------------------

bool CPBSProServer::ConnectToServer(void)
{
    ServerID = pbspro_connect(ServerName);
    if( ServerID <= 0 ){    
        CSmallString error;
        error << "unable to connect to server '" << ServerName << "'";
        ES_TRACE_ERROR(error);
        return(false);
    }
    return(true);
}

//------------------------------------------------------------------------------

bool CPBSProServer::DisconnectFromServer(void)
{
    if( ServerID <= 0 ) return(true);

    int error = pbspro_disconnect(ServerID);
    ServerID = 0;
    if( error != 0 ){
        ES_ERROR("unable to disconnect from server");
        return(false);
    }

    return(true);
}

//------------------------------------------------------------------------------

void CPBSProServer::PrintBatchStatus(std::ostream& sout,struct batch_status* p_bs)
{
    int i = 0;
    while( p_bs != NULL ){
        if( i > 1 ) sout << endl;
        sout << p_bs->name << endl;
        PrintAttributes(sout,p_bs->attribs);
        p_bs = p_bs->next;
        i++;
    }
}

//------------------------------------------------------------------------------

void CPBSProServer::PrintAttributes(std::ostream& sout,struct attrl* p_as)
{
    while( p_as != NULL ){
        sout << "    " << p_as->name;
        if( (p_as->resource != NULL) && (strlen(p_as->resource) > 0) ){
        sout << "." << p_as->resource;
        }
        sout << " = " << p_as->value << endl;
        p_as = p_as->next;
    }
}

//------------------------------------------------------------------------------

void CPBSProServer::PrintAttributes(std::ostream& sout,struct attropl* p_as)
{
    while( p_as != NULL ){
        sout << "    " << p_as->name;
        if( (p_as->resource != NULL) && (strlen(p_as->resource) > 0) ){
        sout << "." << p_as->resource;
        }
        sout << " = " << p_as->value << endl;
        p_as = p_as->next;
    }
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CPBSProServer::UpdateNodes(void)
{
    struct batch_status* p_node_attrs = pbspro_stathost(ServerID,NULL,NULL,NULL);
    if( p_node_attrs == NULL ){
        return(false);
    }

    while( p_node_attrs != NULL ){
        // TDO
        p_node_attrs = p_node_attrs->next;
    }

    pbspro_statfree(p_node_attrs);

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

const CSmallString CPBSProServer::GetLastErrorMsg(void)
{
    CSmallString errmsg(pbspro_geterrmsg(ServerID));
    return(errmsg);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================


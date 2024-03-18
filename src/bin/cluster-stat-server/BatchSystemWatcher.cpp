// =============================================================================
//  Cluster Stat Server
// -----------------------------------------------------------------------------
//     Copyright (C) 2024 Petr Kulhanek (kulhanek@chemi.muni.cz)
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

#include <BatchSystemWatcher.hpp>
#include <ErrorSystem.hpp>
#include <SmallTimeAndDate.hpp>
#include <XMLElement.hpp>
#include <PBSProServer.hpp>
#include <unistd.h>

//------------------------------------------------------------------------------

using namespace std;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CBatchSystemWatcher::CBatchSystemWatcher(void)
{
    PoolingTime         = 20;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CBatchSystemWatcher::ProcessBatchSystemControl(CVerboseStr& vout,CXMLElement* p_config)
{
    vout << "#" << endl;
    vout << "# === [pbs] ====================================================================" << endl;

    if( p_config == NULL ){
        ES_ERROR("no pbs configuration");
        return(false);
    }

    if( p_config->GetAttribute("PBSProLibNames",PBSProLibNames) == true ) {
        vout << "# PBSPro Library Names           = " << PBSProLibNames << endl;
    } else {
        ES_ERROR("no pbs configuration - PBSProLibNames");
        return(false);
    }

    if( p_config->GetAttribute("PBSServerName",PBSServerName) == true ) {
        vout << "# PBSPro Server Name             = " << PBSServerName << endl;
    } else {
        ES_ERROR("no pbs configuration - PBSServerName");
        return(false);
    }

    if( p_config->GetAttribute("PoolingTime",PoolingTime) == true ) {
        vout << "# PBSPro Pooling Time           = " << PoolingTime << endl;
    } else {
        vout << "# PBSPro Pooling Time           = " << PoolingTime << " (default)" << endl;
    }

    PBSVersion = "PBSPro";
    if( p_config->GetAttribute("PBSVersion",PBSVersion) == true ) {
        vout << "# PBS Version                   = " << PBSVersion << endl;
    } else {
        vout << "# PBS Version                   = " << PBSVersion << " (default)" << endl;
    }

    // disable kerberos
    if( PBSVersion == "OpenPBS" ){
        setenv("PBS_AUTH_METHOD","resvport",1);     // OpenPBS
        setenv("PBS_ENCRYPT_METHOD","",1);
    }
    if( PBSVersion == "PBSPro" ){
        setenv("PBSPRO_IGNORE_KERBEROS","yes",1);   // PBSPro
    }

    if( PBSPro.Init(PBSProLibNames,PBSServerName) == false ){
        ES_ERROR("unable to init symbols");
        return(false);
    }

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CBatchSystemWatcher::ExecuteThread(void)
{
    // disable kerberos
    setenv("PBSPRO_IGNORE_KERBEROS","yes",1);

    while( ! ThreadTerminated ) {
        if( PBSPro.ConnectToServer() == true ){
            PBSPro.UpdateNodes();
            PBSPro.DisconnectFromServer();
        }
        sleep(PoolingTime);
    }
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

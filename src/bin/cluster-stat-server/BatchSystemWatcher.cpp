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
    RessurectionTime    = 120;
    Connected           = false;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CBatchSystemWatcher::ProcessBatchSystemControl(CVerboseStr& vout,CXMLElement* p_config)
{
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

    // disable kerberos
    setenv("PBSPRO_IGNORE_KERBEROS","yes",1);

    if( PBSPro.Init(PBSProLibNames,PBSServerName) == true ){
        Connected = true;
        // DEBUG: cout << "PBS - connected" << endl;
    } else {
        ES_WARNING("unable to connect to PBS server");
        // DEBUG: cout << "PBS - not connected" << endl;
        return(true);
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
        if( Connected == false ){
            // DEBUG: cout << "PBS - ressurect" << endl;
            sleep(RessurectionTime);
            if( PBSPro.ConnectToServer() == true ){
                Connected = true;
            } else {
                ES_WARNING("unable to connect to PBS server");
            }
            // DEBUG: cout << "pbs - problem" << endl;
            continue;
        }

        // DEBUG: cout << "pbs - here" << endl;

        if( PBSPro.UpdateNodes() == false ){
            Connected = false;
            continue;
        }

        sleep(PoolingTime);
    }

    if( Connected ){
        PBSPro.DisconnectFromServer();
    }
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

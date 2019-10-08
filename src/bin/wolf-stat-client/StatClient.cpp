// =============================================================================
// wolf-stat-client
// -----------------------------------------------------------------------------
//    Copyright (C) 2019      Petr Kulhanek, kulhanek@chemi.muni.cz
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

#include <XMLParser.hpp>
#include <ErrorSystem.hpp>
#include <FileName.hpp>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fnmatch.h>
#include <StatClient.hpp>
#include <signal.h>

//------------------------------------------------------------------------------

#define MAX_NET_NAME 255

//------------------------------------------------------------------------------

CStatClient StatClient;

MAIN_ENTRY_OBJECT(StatClient)

using namespace std;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CStatClient::CStatClient(void)
{
    Terminated = false;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

int CStatClient::Init(int argc, char* argv[])
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

    return(SO_CONTINUE);
}

//------------------------------------------------------------------------------

bool CStatClient::Run(void)
{
    // CtrlC signal
    signal(SIGINT,CtrlCSignalHandler);
    signal(SIGTERM,CtrlCSignalHandler);

    do {
        Datagram.SetDatagram();
        vout << high;
        Datagram.PrintInfo(vout);

        if( SendDataToServer(Options.GetArgServerName(),Options.GetOptPort()) == false ) {
            ES_ERROR("unable to send datagram");
            return(false);
        }
        if( Options.GetOptInterval() > 0 ){
            sleep(Options.GetOptInterval());
        }

    } while( (Terminated == false) && (Options.GetOptInterval() != 0) );

    return(true);
}

//------------------------------------------------------------------------------

void CStatClient::Finalize(void)
{
    if( Options.GetOptVerbose() ) {
        ErrorSystem.PrintErrors(stderr);
        fprintf(stderr,"\n");
    }
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CStatClient::CtrlCSignalHandler(int signal)
{
    StatClient.vout << endl << endl;
    StatClient.vout << "SIGINT or SIGTERM signal recieved. Initiating server shutdown!" << endl;
    StatClient.vout << "Waiting for server finalization ... " << endl;
    StatClient.Terminated = true;
}

//------------------------------------------------------------------------------

bool CStatClient::SendDataToServer(const CSmallString& servername,int port)
{
    // get IP address of server ---------------------
    addrinfo*      p_addrinfo;
    int            nerr;

    // get hostname and IP
    if( (nerr = getaddrinfo(servername,NULL,NULL,&p_addrinfo)) != 0 ) {
        CSmallString error;
        error << "unable to decode server name '" << servername
              << "' (" <<  gai_strerror(nerr) << ")";
        ES_ERROR(error);
        return(false);
    }

    // get server name
    char s_name[MAX_NET_NAME];
    memset(s_name,0,MAX_NET_NAME);

    if( (nerr = getnameinfo(p_addrinfo->ai_addr,p_addrinfo->ai_addrlen,s_name,
                            MAX_NET_NAME-1,NULL,0,0)) != 0 ) {
        CSmallString error;
        error << "unable to get server name (" <<  gai_strerror(nerr) << ")";
        ES_ERROR(error);
        return(false);
    }
    CSmallString name = s_name;

    // get server IP
    if( (nerr = getnameinfo(p_addrinfo->ai_addr,p_addrinfo->ai_addrlen,s_name,
                            MAX_NET_NAME-1,NULL,0,NI_NUMERICHOST)) != 0 ) {
        CSmallString error;
        error << "unable to get server IP (" <<  gai_strerror(nerr) << ")";
        ES_ERROR(error);
        return(false);
    }
    CSmallString ip = s_name;

    freeaddrinfo(p_addrinfo);

    // Obtain address(es) matching host/port
    sockaddr_in    server_addr;

    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    int sfd;

    sfd = socket(AF_INET,SOCK_DGRAM,0);
    if( sfd == -1 ) {
        CSmallString error;
        error << "unable to create socket (" << strerror(errno) << ")";
        ES_ERROR(error);
        return(false);
    }

    int result = connect(sfd,(sockaddr*)&server_addr,sizeof(server_addr));

    if( result == -1 ) {
        CSmallString error;
        error << "unable to connect to server " << name
              << "[" << ip << "] (" << strerror(errno) << ")";
        ES_ERROR(error);
        return(false);
    }

    // send module datagram to server

    if (send(sfd, &Datagram, sizeof(Datagram), MSG_NOSIGNAL) != sizeof(Datagram)) {
        ES_ERROR("unable to send datagram");
        exit(EXIT_FAILURE);
    }

    // close stream
    close(sfd);

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

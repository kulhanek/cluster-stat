// =============================================================================
//  AMS - Advanced Module System - Common Library
// -----------------------------------------------------------------------------
//     Copyright (C) 2004,2005,2008 Petr Kulhanek (kulhanek@chemi.muni.cz)
//
//     This library is free software; you can redistribute it and/or
//     modify it under the terms of the GNU Lesser General Public
//     License as published by the Free Software Foundation; either
//     version 2.1 of the License, or (at your option) any later version.
//
//     This library is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//     Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public
//     License along with this library; if not, write to the Free Software
//     Foundation, Inc., 51 Franklin Street, Fifth Floor,
//     Boston, MA  02110-1301  USA
// =============================================================================

#include <ErrorSystem.hpp>
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
#include <StatServer.hpp>
#include <StatDatagram.hpp>
#include <FCGIStatServer.hpp>

//------------------------------------------------------------------------------

#define MAX_NET_NAME 255

using namespace std;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CStatServer::CStatServer(void)
{
    Socket = -1;
    Port = 32598;
    AllRequests = 0;
    SuccessfulRequests = 0;
}

//------------------------------------------------------------------------------

CStatServer::~CStatServer(void)
{
    if( Socket != -1 ) close(Socket);
}

//------------------------------------------------------------------------------

void CStatServer::SetPort(int port)
{
    Port = port;
}

//------------------------------------------------------------------------------

void CStatServer::TerminateServer(void)
{
    if( Socket != -1 ) shutdown(Socket, SHUT_RDWR);
    TerminateThread();
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

void CStatServer::ExecuteThread(void)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int  s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL,CSmallString(Port), &hints, &result);
    if(s != 0) {
        CSmallString error;
        error << "getaddrinfo: " << gai_strerror(s);
        ES_ERROR(error);
        return;
    }

    //  getaddrinfo() returns a list of address structures.
    // Try each address until we successfully bind(2).
    // If socket(2) (or bind(2)) fails, we (close the socket
    // and) try the next address.

    for(rp = result; rp != NULL; rp = rp->ai_next) {
        Socket = socket(rp->ai_family, rp->ai_socktype,
                        rp->ai_protocol);
        if( Socket == -1 ) continue;

        if( bind(Socket, rp->ai_addr, rp->ai_addrlen) == 0) break; // Success

        close(Socket);
        Socket = -1;
    }

    if( rp == NULL ) { // No address succeeded
        ES_ERROR("could not bind");
        return;
    }

    freeaddrinfo(result); // No longer needed

    // server loop
    while( (ThreadTerminated == false) && (Socket != -1) ) {

        CStatDatagram           datagram;
        struct sockaddr_storage peer_addr;
        socklen_t               peer_addr_len;
        ssize_t                 nread;

        // get datagram ------------------------------
        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(Socket,&datagram,sizeof(datagram), 0,
                         (struct sockaddr *)&peer_addr, &peer_addr_len);

        AllRequests++;

        if(nread == -1) continue;                   // Ignore failed request
        if(nread != sizeof(datagram) ) continue;    // Ignore incomplete request

        char host[NI_MAXHOST], service[NI_MAXSERV];
        memset(host,0,NI_MAXHOST);

        // get client hostname -----------------------
        s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST-1,
                        service, NI_MAXSERV-1, NI_NUMERICSERV);
        if(s != 0) {
            // client hostname is not available
            CSmallString error;
            error << "getnameinfo: " << gai_strerror(s);
            ES_ERROR(error);
            continue;
        }

        // validate datagram -------------------------
        if( datagram.IsValid() == false ) {
            ES_ERROR("datagram is not valid (checksum error)");
            continue;
        }

        ClusterStatServer.RegisterNode(datagram);

        SuccessfulRequests++;
    }

    return;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

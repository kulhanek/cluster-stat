#ifndef AMSStatServerH
#define AMSStatServerH
// =============================================================================
// AMS - Advanced Module System
// -----------------------------------------------------------------------------
//    Copyright (C) 2004,2005,2008 Petr Kulhanek, kulhanek@chemi.muni.cz
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

#include <SmartThread.hpp>

//------------------------------------------------------------------------------

class CStatServer : public CSmartThread  {
public:
// constructor and destructors -------------------------------------------------
    CStatServer(void);
    ~CStatServer(void);

    ///! set server port
    void SetPort(int port);

    //! terminate server, e.g. close socket and termineate thread
    void TerminateServer(void);

public:
    int AllRequests;
    int SuccessfulRequests;

// section of private data -----------------------------------------------------
private:
    int Port;
    int Socket;

// execute server --------------------------------------------------------------
    //! execute server
    virtual void ExecuteThread(void);

};

// -----------------------------------------------------------------------------

#endif

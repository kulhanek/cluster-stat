#ifndef PBSProServerH
#define PBSProServerH
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

#include <DynamicPackage.hpp>
#include <iostream>

// -----------------------------------------------------------------------------

class CQueueList;
class CNodeList;
class CJobList;
class CJob;

// -----------------------------------------------------------------------------

typedef int (*PBS_CONNECT)(const char*);
typedef int (*PBS_DISCONNECT)(int);
typedef struct batch_status* (*PBS_STATSERVER)(int,struct attrl*,char*);
typedef struct batch_status* (*PBS_STATQUE)(int,char*,struct attrl*,char*);
typedef struct batch_status* (*PBS_STATHOST)(int,char*,struct attrl*,char*);
typedef struct batch_status* (*PBS_STATVNODE)(int,char*,struct attrl*,char*);
typedef struct batch_status* (*PBS_STATJOB)(int,char*,struct attrl*,char*);
// the last argument is due to PBSPro
typedef struct batch_status* (*PBS_SELSTATJOB)(int,struct attropl*,char*,char*);
typedef void (*PBS_STATFREE)(struct batch_status *);
typedef char* (*PBS_GETERRMSG)(int connect);
typedef char* (*PBS_LOCJOB)(int,char*,char*);
typedef char* (*PBS_STRERROR)(int);
typedef int (*PBS_ERRNO)(void);

// -----------------------------------------------------------------------------

class CPBSProServer {
public:
// constructor -----------------------------------------------------------------
        CPBSProServer(void);
        ~CPBSProServer(void);

// init batch system subsystem -------------------------------------------------------
    //! load symbols and connect to server
    bool Init(const CSmallString& libs_tok,const CSmallString& server_name);

    //! connect to server
    bool ConnectToServer(void);

    //! disconnect from server
    bool DisconnectFromServer(void);

// execution -------------------------------------------------------------------
    //! get last error message
    const CSmallString GetLastErrorMsg(void);

    //! update node statuses
    bool UpdateNodes(void);

// section of private data -----------------------------------------------------
private:
    CSmallString    ServerName;
    CSmallString    PBSProLibName;
    CDynamicPackage PBSProLib;
    int             ServerID;

    // init symbols
    bool InitSymbols(void);

    // print batch_status
    void PrintBatchStatus(std::ostream& sout,struct batch_status* p_bs);

    // print attributes
    void PrintAttributes(std::ostream& sout,struct attrl* p_as);

    // print attributes
    void PrintAttributes(std::ostream& sout,struct attropl* p_as);

    // batch system api symbols
    PBS_CONNECT     pbspro_connect;
    PBS_DISCONNECT  pbspro_disconnect;
    PBS_STATSERVER  pbspro_statserver;
    PBS_STATQUE     pbspro_statque;
    PBS_STATHOST    pbspro_stathost;
    PBS_STATVNODE   pbspro_statvnode;
    PBS_STATJOB     pbspro_statjob;
    PBS_SELSTATJOB  pbspro_selstat;
    PBS_STATFREE    pbspro_statfree;
    PBS_GETERRMSG   pbspro_geterrmsg;
    PBS_STRERROR    pbspro_strerror;
    PBS_LOCJOB      pbspro_locjob;
    PBS_ERRNO       pbspro_errno;
};

// -----------------------------------------------------------------------------

extern CPBSProServer PBSPro;

// -----------------------------------------------------------------------------

#endif

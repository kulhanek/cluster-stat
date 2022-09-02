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
#include "PBSProAttr.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <FileSystem.hpp>
#include <NodeList.hpp>
#include <JobList.hpp>
#include "PBSProQueue.hpp"
#include "PBSProNode.hpp"
#include "PBSProJob.hpp"
#include <XMLElement.hpp>

using namespace std;
using namespace boost;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CPBSProServer::CPBSProServer(void)
    : CBatchServer()
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

bool CPBSProServer::Init(const CSmallString& server_name,const CSmallString& short_name,
                         const CSmallString& alt_names,bool job_transfer)
{
    CSmallString libs_tok;

// FIXME
//    if( ABSConfig.GetSystemConfigItem("INF_PBSPRO_LIBS",libs_tok) == false ){
//        ES_ERROR("unable to get paths to PBSPro client library");
//        return(false);
//    }

    StartTimer();

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
        EndTimer();
        return(false);
    }

    ServerName = server_name;
    ShortName = short_name;
    AltNames = alt_names;
    AllowJobTransfer = job_transfer;

    if( ConnectToServer() == false ){
        ES_TRACE_ERROR("unable to connect to server");
        EndTimer();
        return(false);
    }

    EndTimer();
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

const CSmallString CPBSProServer::LocateJob(const CSmallString& jobid)
{
    if( AllowJobTransfer == false ){
        return(ServerName);
    }

    CSmallString full_job_id;
    full_job_id = GetFullJobID(jobid);

    char* p_srv_name = pbspro_locjob(ServerID,(char*)full_job_id.GetBuffer(),NULL);
    if( p_srv_name == NULL ) return("");
    CSmallString srv_name(p_srv_name);
    free(p_srv_name);
    return(srv_name);
}

//------------------------------------------------------------------------------

bool CPBSProServer::GetQueues(CQueueList& queues)
{
    StartTimer();

    struct batch_status* p_queue_attrs = pbspro_statque(ServerID,NULL,NULL,NULL);

    bool result = true;
    while( p_queue_attrs != NULL ){
        CPBSProQueue* p_queue = new CPBSProQueue();
        if( p_queue->Init(ShortName, p_queue_attrs) == false ){
            ES_ERROR("unable to init queue");
            result = false;
            delete p_queue;
        } else {
            CQueuePtr queue(p_queue);
            queues.push_back(queue);
        }
        p_queue_attrs = p_queue_attrs->next;
    }

    if( p_queue_attrs ) pbspro_statfree(p_queue_attrs);

    EndTimer();

    return(result);
}

//------------------------------------------------------------------------------

bool CPBSProServer::GetNodes(CNodeList& nodes)
{
    StartTimer();

    struct batch_status* p_node_attrs = pbspro_stathost(ServerID,NULL,NULL,NULL);

    bool result = true;
    while( p_node_attrs != NULL ){
        CPBSProNode* p_node = new CPBSProNode;
        if( p_node->Init(ServerName,ShortName,p_node_attrs) == false ){
            ES_ERROR("unable to init node");
            result = false;
            delete p_node;
        } else {
            CNodePtr node(p_node);
            nodes.push_back(node);
        }
        p_node_attrs = p_node_attrs->next;
    }

    if( p_node_attrs ) pbspro_statfree(p_node_attrs);

    EndTimer();

    return(result);
}

//------------------------------------------------------------------------------

const CNodePtr CPBSProServer::GetNode(const CSmallString& name)
{
    StartTimer();

    CNodePtr node;

    struct batch_status* p_node_attrs = pbspro_stathost(ServerID,(char*)name.GetBuffer(),NULL,NULL);
    if( p_node_attrs != NULL ) {
        CPBSProNode* p_node = new CPBSProNode;
        if( p_node->Init(ServerName,ShortName,p_node_attrs) == false ){
            ES_ERROR("unable to init node");
            delete p_node;
        } else {
            CNodePtr lnode(p_node);
            node = lnode;
        }
        pbspro_statfree(p_node_attrs);
    } else {
        // try vhost
        p_node_attrs = pbspro_statvnode(ServerID,(char*)name.GetBuffer(),NULL,NULL);
        if( p_node_attrs != NULL ) {
            CPBSProNode* p_node = new CPBSProNode;
            if( p_node->Init(ServerName,ShortName,p_node_attrs) == false ){
                ES_ERROR("unable to init node");
                delete p_node;
            } else {
                CNodePtr lnode(p_node);
                node = lnode;
            }
            pbspro_statfree(p_node_attrs);
        }
    }

    EndTimer();

    return(node);
}

//------------------------------------------------------------------------------

bool CPBSProServer::GetAllJobs(CJobList& jobs,bool include_history)
{
    StartTimer();

    CSmallString extend;
    if( include_history ){
        extend << "x";
    }

    struct batch_status* p_jobs;
    p_jobs = pbspro_statjob(ServerID,NULL,NULL,extend.GetBuffer());

    bool result = true;
    while( p_jobs != NULL ){
        CPBSProJob* p_job = new CPBSProJob;
        if( p_job->Init(ServerName,ShortName,p_jobs) == false ){
            ES_ERROR("unable to init job");
            result = false;
            delete p_job;
        } else {
            CJobPtr job(p_job);
            jobs.push_back(job);
        }
        p_jobs = p_jobs->next;
    }

    if( p_jobs ) pbspro_statfree(p_jobs);

    EndTimer();

    return(result);
}

//------------------------------------------------------------------------------

bool CPBSProServer::GetQueueJobs(CJobList& jobs,const CSmallString& queue_name,bool include_history)
{
    StartTimer();

    CSmallString extend;
    if( include_history ){
        extend << "x";
    }

    struct batch_status* p_jobs;
    p_jobs = pbspro_statjob(ServerID,(char*)queue_name.GetBuffer(),NULL,extend.GetBuffer());

    bool result = true;
    while( p_jobs != NULL ){
        CPBSProJob* p_job = new CPBSProJob;
        if( p_job->Init(ServerName,ShortName,p_jobs) == false ){
            ES_ERROR("unable to init job");
            result = false;
            delete p_job;
        } else {
            CJobPtr job(p_job);
            jobs.push_back(job);
        }
        p_jobs = p_jobs->next;
    }

    if( p_jobs ) pbspro_statfree(p_jobs);

    EndTimer();

    return(result);
}

//------------------------------------------------------------------------------

bool CPBSProServer::GetUserJobs(CJobList& jobs,const CSmallString& user,bool include_history)
{
    StartTimer();

    CSmallString extend;
    if( include_history ){
        extend << "x";
    }

    struct attropl* p_first = NULL;
    set_attribute(p_first,ATTR_u,NULL,user,EQ);

    struct batch_status* p_jobs;
    p_jobs = pbspro_selstat(ServerID,p_first,NULL,extend.GetBuffer());

    bool result = true;
    while( p_jobs != NULL ){
        CPBSProJob* p_job = new CPBSProJob;
        if( p_job->Init(ServerName,ShortName,p_jobs) == false ){
            ES_ERROR("unable to init job");
            result = false;
            delete p_job;
        } else {
            CJobPtr job(p_job);
            jobs.push_back(job);
        }
        p_jobs = p_jobs->next;
    }

    if( p_jobs ) pbspro_statfree(p_jobs);

    EndTimer();

    return(result);
}

//------------------------------------------------------------------------------

bool CPBSProServer::GetJob(CJobList& jobs,const CSmallString& jobid)
{
    CJobPtr job = GetJob(jobid);
    if( job != NULL ){
        jobs.push_back(job);
        return(true);
    }
    return(false);
}

//------------------------------------------------------------------------------

const CJobPtr CPBSProServer::GetJob(const CSmallString& jobid)
{
    StartTimer();

    // always use extend attribute
    CSmallString extend;
    extend << "x";

    CSmallString full_job_id;
    full_job_id = GetFullJobID(jobid);

    struct batch_status* p_jobs;
    p_jobs = pbspro_statjob(ServerID,(char*)full_job_id.GetBuffer(),NULL,extend.GetBuffer());

    if( p_jobs == NULL ){
        char* p_error = pbspro_geterrmsg(ServerID);
        if( p_jobs ) ES_TRACE_ERROR(p_error);
    }

    CJobPtr result;

    if( p_jobs != NULL ){
        CPBSProJob* p_job = new CPBSProJob;
        if( p_job->Init(ServerName,ShortName,p_jobs) == false ){
            delete p_job;
            ES_ERROR("unable to init job");
        } else {
            result = CJobPtr(p_job);
        }
    }

    if( p_jobs ) pbspro_statfree(p_jobs);

    EndTimer();

    return(result);
}

//------------------------------------------------------------------------------

bool CPBSProServer::PrintQueues(std::ostream& sout)
{
    StartTimer();

    struct batch_status* p_queues = pbspro_statque(ServerID,NULL,NULL,NULL);
    if( p_queues != NULL ) {
        PrintBatchStatus(sout,p_queues);
        pbspro_statfree(p_queues);
    }

    EndTimer();

    return(p_queues != NULL);
}

//------------------------------------------------------------------------------

bool CPBSProServer::PrintNodes(std::ostream& sout)
{
    StartTimer();

    struct batch_status* p_nodes = pbspro_stathost(ServerID,NULL,NULL,NULL);
    if( p_nodes != NULL ) {
        PrintBatchStatus(sout,p_nodes);
        pbspro_statfree(p_nodes);
    }

    EndTimer();

    return(p_nodes != NULL);
}

//------------------------------------------------------------------------------

bool CPBSProServer::PrintNode(std::ostream& sout,const CSmallString& name)
{
    StartTimer();

    struct batch_status* p_nodes = pbspro_stathost(ServerID,(char*)name.GetBuffer(),NULL,NULL);
    if( p_nodes != NULL ) {
        PrintBatchStatus(sout,p_nodes);
        pbspro_statfree(p_nodes);
    } else {
        // try vhost
        p_nodes = pbspro_statvnode(ServerID,(char*)name.GetBuffer(),NULL,NULL);
        if( p_nodes != NULL ) {
            PrintBatchStatus(sout,p_nodes);
            pbspro_statfree(p_nodes);
        }
    }

    EndTimer();

    return(p_nodes != NULL);
}

//------------------------------------------------------------------------------

bool CPBSProServer::PrintJobs(std::ostream& sout,bool include_history)
{
    StartTimer();

    CSmallString extend;
    if( include_history ){
        extend << "x";
    }

    struct batch_status* p_jobs = pbspro_statjob(ServerID,NULL,NULL,extend.GetBuffer());
    if( p_jobs != NULL ) {
        PrintBatchStatus(sout,p_jobs);
        pbspro_statfree(p_jobs);
    }

    EndTimer();

    return(p_jobs != NULL);
}

//------------------------------------------------------------------------------

bool CPBSProServer::PrintJob(std::ostream& sout,const CSmallString& jobid,bool include_history)
{
    StartTimer();

    CSmallString extend;
    if( include_history ){
        extend << "x";
    }

    CSmallString full_job_id;
    full_job_id = GetFullJobID(jobid);

    struct batch_status* p_jobs = pbspro_statjob(ServerID,(char*)full_job_id.GetBuffer(),NULL,extend.GetBuffer());
    if( p_jobs != NULL ) {
        PrintBatchStatus(sout,p_jobs);
        pbspro_statfree(p_jobs);
    }

    EndTimer();

    return(p_jobs != NULL);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

//enum EJobStatus {
//    EJS_NONE,
//    EJS_PREPARED,       // prepared job
//    EJS_SUBMITTED,      // Q+H+T
//    EJS_BOOTING,        // R but not pinfo
//    EJS_RUNNING,        // R+S+U+B
//    EJS_FINISHED,       // F+E
//    EJS_KILLED,         // killed
//    EJS_ERROR,          // internal error
//    EJS_MOVED,          // M
//    EJS_INCONSISTENT    // inconsistency
//};


bool CPBSProServer::GetJobStatus(CJob& job)
{
    StartTimer();

    CSmallString jobid = job.GetJobID();

    batch_status* p_status = pbspro_statjob(ServerID,jobid.GetBuffer(),NULL,NULL);

    job.BatchJobComment = NULL;
    job.BatchJobStatus = EJS_ERROR;

    if( p_status == NULL ){
        return(true);
    }

    attrl* p_item = p_status->attribs;

    CPBSProJob::DecodeBatchJobComment(p_item,job.BatchJobComment);

    while( p_item != NULL ){
        // job status
        if( strcmp(p_item->name,"job_state") == 0 ){
            CSmallString status;
            status = p_item->value;
            if( status == "Q" ){
                job.BatchJobStatus = EJS_SUBMITTED;
            } else if( status == "H" ) {
                job.BatchJobStatus = EJS_SUBMITTED;
            } else if( status == "T" ) {
                job.BatchJobStatus = EJS_SUBMITTED;
            } else if( status == "R" ) {
                job.BatchJobStatus = EJS_RUNNING;
            } else if( status == "S" ) {
                job.BatchJobStatus = EJS_RUNNING;
            } else if( status == "B" ) {
                job.BatchJobStatus = EJS_RUNNING;
            } else if( status == "U" ) {
                job.BatchJobStatus = EJS_RUNNING;
            } else if( status == "E" ) {
                job.BatchJobStatus = EJS_FINISHED;
            } else if( status == "F" ) {
                job.BatchJobStatus = EJS_FINISHED;
            } else if( status == "M" ) {
                job.BatchJobStatus = EJS_MOVED;
            }
        }

        p_item = p_item->next;
    }

    // to handle moved jobs
    CSmallString tmp;
    tmp = NULL;
    get_attribute(p_status->attribs,ATTR_queue,NULL,tmp);
    job.SetItem("specific/resources","INF_REAL_QUEUE",tmp);

    tmp = NULL;
    get_attribute(p_status->attribs,ATTR_server,NULL,tmp);
    job.SetItem("specific/resources","INF_REAL_SERVER",tmp);

    pbspro_statfree(p_status);

    EndTimer();

    return(true);
}

//------------------------------------------------------------------------------

const CSmallString CPBSProServer::GetLastErrorMsg(void)
{
    StartTimer();
    CSmallString errmsg(pbspro_geterrmsg(ServerID));
    EndTimer();

    return(errmsg);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================


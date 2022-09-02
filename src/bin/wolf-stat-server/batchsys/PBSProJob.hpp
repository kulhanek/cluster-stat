#ifndef PBSProJobH
#define PBSProJobH
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

#include <Job.hpp>
#include <XMLDocument.hpp>

// -----------------------------------------------------------------------------

/// PBSPro job

class CPBSProJob : private CXMLDocument {
public:
// constructor -----------------------------------------------------------------
        CPBSProJob(void);

// methods ---------------------------------------------------------------------
    //! init job with batch system information
    bool Init(const CSmallString& srv_name,const CSmallString& short_srv_name,struct batch_status* p_job);

    //! decode job comment
    static void DecodeBatchJobComment(struct attrl* p_item,CSmallString& comment);

// information methods ---------------------------------------------------------
    /// 0.0 create info file header
    void CreateHeaderFromBatchJob(const CSmallString& siteid, const CSmallString& absvers);

    /// create section
    bool CreateSection(const CSmallString& section);

    /// get job configuration item
    const CSmallString GetItem(const CSmallString& path,
                               const CSmallString& name,bool noerror=false);

    /// get job configuration item
    bool GetItem(const CSmallString& path,
                 const CSmallString& name,
                 CSmallString& value,bool noerror=false);

    /// set job configuration item
    void SetItem(const CSmallString& path,
                 const CSmallString& name,
                 const CSmallString& value);

    /// get job configuration item
    bool GetTime(const CSmallString& path,
                 const CSmallString& name,
                 CSmallTime& value,bool noerror=false);

    /// set job configuration item
    void SetTime(const CSmallString& path,
                 const CSmallString& name,
                 const CSmallTime& value);

// section of private data
private:
    CSmallString                        BatchServerName;
    CSmallString                        ShortServerName;
    EJobStatus                          BatchJobStatus;
    CSmallString                        BatchJobComment;
    std::map<std::string,std::string>   BatchVariables;
    std::set<std::string>               ExecVNodes;         // unique list of vnodes
};

// -----------------------------------------------------------------------------

#endif

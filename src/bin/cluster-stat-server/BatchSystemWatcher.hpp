#ifndef BatchSystemWatcherH
#define BatchSystemWatcherH
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

#include <SmartThread.hpp>
#include <SmallTime.hpp>
#include <VerboseStr.hpp>
#include <XMLElement.hpp>

//------------------------------------------------------------------------------

class CBatchSystemWatcher : public CSmartThread {
public:
// constructor -----------------------------------------------------------------
    CBatchSystemWatcher(void);

    //! read common setup
    bool ProcessBatchSystemControl(CVerboseStr& vout,CXMLElement* p_config);

// section of private data -----------------------------------------------------
private:
    CSmallString    PBSProLibNames;
    CSmallString    PBSServerName;
    int             PoolingTime;
    CSmallString    PBSVersion;     // PBSPro, OpenPBS

    // main loop
    virtual void ExecuteThread(void);
};

//------------------------------------------------------------------------------

#endif

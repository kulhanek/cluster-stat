// =============================================================================
//  WOLF Stat Server
// -----------------------------------------------------------------------------
//     Copyright (C) 2015 Petr Kulhanek (kulhanek@chemi.muni.cz)
//     Copyright (C) 2012 Petr Kulhanek (kulhanek@chemi.muni.cz)
//     Copyright (C) 2011      Petr Kulhanek, kulhanek@chemi.muni.cz
//     Copyright (C) 2001-2008 Petr Kulhanek, kulhanek@chemi.muni.cz
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

#include "WolfStatServer.hpp"
#include <TemplateParams.hpp>
#include <ErrorSystem.hpp>
#include <FileName.hpp>
#include <SmallTimeAndDate.hpp>
#include <SmallTime.hpp>
#include <DirectoryEnum.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace boost;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool CWolfStatServer::_ListLoggedUsers(CFCGIRequest& request)
{
    CFileName           dir = GetLoggedUserStatDir();
    CDirectoryEnum      edir;
    vector<CFileName>   nodes;

    edir.SetDirName(dir);
    edir.StartFindFile(GetLoggedUserStatFilter());

    CFileName node;
    while( edir.FindFile(node) ){
        nodes.push_back(node);
    }

    edir.EndFindFile();

    vector<CFileName>::iterator it = nodes.begin();
    vector<CFileName>::iterator ie = nodes.end();

    while( it != ie ){
        CSmallString full_name = *it;
        CSmallString short_name = GetShortName(full_name);

        ifstream ifs(dir / full_name);
        string          line;
        vector<string>  words;

        if( ifs ){
            getline(ifs,line);
            split(words,line,is_any_of(";"));
            ifs.close();
        }

        // check node status
        CSmallString status = "up";
        // check timestamp from user stat file
        if( words.size() > 0 ){
            CSmallString stimestamp = words[0];
            CSmallTimeAndDate stime(stimestamp.ToInt());
            CSmallTimeAndDate ctime;
            ctime.GetActualTimeAndDate();
            CSmallTime diff = ctime - stime;
            if( diff > 300 ){  // skew of 300 seconds
                status = "down";
            }
        } else {
            status = "down"; // stat file does not exist
        }

        // write response
        request.OutStream.PutStr(status); // node status
        request.OutStream.PutChar(';');
        request.OutStream.PutStr(short_name); // node name
        if( words.size() == 3 ){
            request.OutStream.PutChar(';');
            request.OutStream.PutStr(words[1].c_str()); // full user name - optional
            request.OutStream.PutChar(';');
            request.OutStream.PutStr(words[2].c_str()); // login name - optional
        }
        request.OutStream.PutChar('\n');

        it++;
    }

    // finalize request
    request.FinishRequest();

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

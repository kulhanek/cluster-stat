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

#include <StatDatagram.hpp>
#include <string.h>
#include <SmallTimeAndDate.hpp>
#include <string>
#include <vector>
#include <utmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

//------------------------------------------------------------------------------

using namespace std;
using namespace boost;

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

CStatDatagram::CStatDatagram(void)
{
    memset(Header,0,HEADER_SIZE);
    memset(NodeName,0,NAME_SIZE);
    memset(UserName,0,NAME_SIZE);
    memset(LoginName,0,NAME_SIZE);
    IUsers = 0;
    RUsers = 0;
    TimeStamp = 0;
    CheckSum = 0;
}

//------------------------------------------------------------------------------

void CStatDatagram::SetDatagram(void)
{
    memset(Header,0,HEADER_SIZE);
    memset(NodeName,0,NAME_SIZE);
    memset(UserName,0,NAME_SIZE);
    memset(LoginName,0,NAME_SIZE);
    IUsers = 0;
    RUsers = 0;
    TimeStamp = 0;
    CheckSum = 0;

    strncpy(Header,"STAT",4);

    gethostname(NodeName,NAME_SIZE-1);

    setutent();

    struct utmp* my_utmp;

    while( (my_utmp = getutent()) != NULL) {
        if( my_utmp->ut_type == USER_PROCESS ){
            char line[UT_LINESIZE+1];
            memset(line,0,UT_LINESIZE+1);
            strncpy(line,my_utmp->ut_line,UT_LINESIZE);
            if( strstr(line,":") != NULL ){
                char login[UT_NAMESIZE+1];
                memset(login,0,UT_NAMESIZE+1);
                strncpy(login,my_utmp->ut_user,UT_NAMESIZE);
                struct passwd* my_passwd = getpwnam(login);
                if( my_passwd != NULL ){
                    CSmallString name;
                    std::string stext = std::string(my_passwd->pw_gecos);
                    vector<string>  words;
                    split(words,stext,is_any_of(","));
                    if( words.size() >= 1 ){
                        name = words[0];
                    }
                    strncpy(UserName,name,NAME_SIZE-1);
                }
                strncpy(LoginName,login,NAME_SIZE-1);
                IUsers++;
            } else {
                RUsers++;
            }
        }
    }

    endutent();

    CSmallTimeAndDate time;
    time.GetActualTimeAndDate();
    TimeStamp = time.GetSecondsFromBeginning();

    // calculate checksum
    for(size_t i=0; i < HEADER_SIZE; i++){
        CheckSum += Header[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        CheckSum += (unsigned char)NodeName[i];
        CheckSum += (unsigned char)UserName[i];
        CheckSum += (unsigned char)LoginName[i];
    }
    CheckSum += IUsers;
    CheckSum += RUsers;
    CheckSum += TimeStamp;
}

//------------------------------------------------------------------------------

bool CStatDatagram::IsValid(void)
{
    int checksum = 0;

    // calculate checksum
    for(size_t i=0; i < HEADER_SIZE; i++){
        checksum += Header[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        checksum += (unsigned char)NodeName[i];
        checksum += (unsigned char)UserName[i];
        checksum += (unsigned char)LoginName[i];
    }
    checksum += IUsers;
    checksum += RUsers;
    checksum += TimeStamp;

    return( checksum == CheckSum );
}

//------------------------------------------------------------------------------

void CStatDatagram::PrintInfo(std::ostream& vout)
{
    vout << "Node  = " << GetShortNodeName() << endl;
    vout << "User  = " << GetUserName() << endl;
    vout << "Login = " << GetLoginName() << endl;
    vout << "I#    = " << IUsers << endl;
    vout << "R#    = " << RUsers << endl;
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetShortNodeName(void)
{
    NodeName[NAME_SIZE-1] = '\0';
    CSmallString fqdn = NodeName;

    std::string stext = std::string(fqdn);
    vector<string>  words;
    split(words,stext,is_any_of("."));
    if( words.size() >= 1 ) return( words[0] );
    return("");
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetNodeName(void)
{
    NodeName[NAME_SIZE-1] = '\0';
    return(NodeName);
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetUserName(void)
{
    UserName[NAME_SIZE-1] = '\0';
    return(UserName);
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetLoginName(void)
{
    LoginName[NAME_SIZE-1] = '\0';
    return(LoginName);
}

//------------------------------------------------------------------------------

int CStatDatagram::GetTimeStamp(void)
{
    return(TimeStamp);
}

//------------------------------------------------------------------------------

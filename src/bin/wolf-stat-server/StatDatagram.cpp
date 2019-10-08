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
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <sstream>

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
    memset(UserName,0,NAME_SIZE*MAX_TTYS);
    memset(LoginName,0,NAME_SIZE*MAX_TTYS);
    ActiveTTY = 0;
    NumOfIUsers = 0;
    NumOfAUsers = 0;
    TimeStamp = 0;
    CheckSum = 0;
}

//------------------------------------------------------------------------------

void CStatDatagram::SetDatagram(void)
{
    memset(Header,0,HEADER_SIZE);
    memset(NodeName,0,NAME_SIZE);
    memset(UserName,0,NAME_SIZE*MAX_TTYS);
    memset(LoginName,0,NAME_SIZE*MAX_TTYS);
    ActiveTTY = 0;
    NumOfIUsers = 0;
    NumOfAUsers = 0;
    TimeStamp = 0;
    CheckSum = 0;

    strncpy(Header,"STAT",4);

    gethostname(NodeName,NAME_SIZE-1);

    // get list of local session
    FILE* p_sf = popen("/bin/loginctl --no-legend --no-pager list-sessions","r");

    CSmallString buffer;
    while( (p_sf != NULL) && (buffer.ReadLineFromFile(p_sf,true,true)) ) {
        NumOfAUsers++;
        stringstream str(buffer.GetBuffer());
        string sid;
        string uid;
        string user;
        string seat;
        string tty;
        str >> sid >> uid >> user >> seat >> tty;
        if( seat == "seat0" ){
            // decode user
            char login[NAME_SIZE+1];
            memset(login,0,NAME_SIZE+1);
            strncpy(login,user.c_str(),NAME_SIZE);
            struct passwd* my_passwd = getpwnam(login);
            // only regular users
            if( (my_passwd != NULL) && (my_passwd->pw_uid > 1000) ){
                CSmallString name;
                std::string stext = std::string(my_passwd->pw_gecos);
                vector<string>  words;
                split(words,stext,is_any_of(","));
                if( words.size() >= 1 ){
                    name = words[0];
                }
                // decode tty
                char line[NAME_SIZE+1];
                memset(line,0,NAME_SIZE+1);
                strncpy(line,tty.c_str(),NAME_SIZE);
                line[0] = ' ';
                line[1] = ' ';
                line[2] = ' ';
                stringstream str(line);
                int tty = 0;
                str >> tty;
                if( (tty > 0) && (tty <= MAX_TTYS) ){
                    tty--;
                    NumOfIUsers++;
                    strncpy(UserName[tty],name,NAME_SIZE-1);
                    strncpy(LoginName[tty],login,NAME_SIZE-1);
                }
            }

        }
    }

    if( p_sf ) fclose(p_sf);

    // load active tty
    FILE* p_tty0 = fopen("/sys/class/tty/tty0/active","r");
    if( p_tty0 ){
        char ttyname[NAME_SIZE+1];
        memset(ttyname,0,NAME_SIZE+1);
        fgets(ttyname,NAME_SIZE,p_tty0);
        if( strstr(ttyname,"tty") != NULL ){
            ttyname[0] = ' ';
            ttyname[1] = ' ';
            ttyname[2] = ' ';
            stringstream str(ttyname);
            str >> ActiveTTY;
            if( (ActiveTTY <= 0) || (ActiveTTY > MAX_TTYS) ){
                ActiveTTY = 0;
            }
        }
        fclose(p_tty0);
    }

    CSmallTimeAndDate time;
    time.GetActualTimeAndDate();
    TimeStamp = time.GetSecondsFromBeginning();

    // calculate checksum
    for(size_t i=0; i < HEADER_SIZE; i++){
        CheckSum += Header[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        CheckSum += (unsigned char)NodeName[i];
    }
    for(size_t k=0; k < MAX_TTYS; k++){
        for(size_t i=0; i < NAME_SIZE; i++){
            CheckSum += (unsigned char)UserName[k][i];
            CheckSum += (unsigned char)LoginName[k][i];
        }
    }

    CheckSum += ActiveTTY;
    CheckSum += NumOfIUsers;
    CheckSum += NumOfAUsers;
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
    }
    for(size_t k=0; k < MAX_TTYS; k++){
        for(size_t i=0; i < NAME_SIZE; i++){
            checksum += (unsigned char)UserName[k][i];
            checksum += (unsigned char)LoginName[k][i];
        }
    }

    checksum += ActiveTTY;
    checksum += NumOfIUsers;
    checksum += NumOfAUsers;
    checksum += TimeStamp;

    return( checksum == CheckSum );
}

//------------------------------------------------------------------------------

void CStatDatagram::PrintInfo(std::ostream& vout)
{
    vout << "Node  = " << GetShortNodeName() << endl;
    vout << "User  = " << GetUserName() << endl;
    vout << "Login = " << GetLoginName() << endl;
    vout << "ATTY  = " << ActiveTTY << endl;
    vout << "I#U   = " << NumOfIUsers << endl;
    vout << "T#U   = " << NumOfAUsers << endl;
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
    if( (ActiveTTY > 0) && (ActiveTTY <= MAX_TTYS) ){
        size_t id = ActiveTTY-1;
        UserName[id][NAME_SIZE-1] = '\0';
        return(UserName[id]);
    }
    return("");
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetLoginName(void)
{
    if( (ActiveTTY > 0) && (ActiveTTY <= MAX_TTYS) ){
        size_t id = ActiveTTY-1;
        LoginName[id][NAME_SIZE-1] = '\0';
        return(LoginName[id]);
    }
    return("");
}

//------------------------------------------------------------------------------

int CStatDatagram::GetTimeStamp(void)
{
    return(TimeStamp);
}

//------------------------------------------------------------------------------

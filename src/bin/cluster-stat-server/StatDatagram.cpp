// =============================================================================
// cluster-stat-client
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
#include <FileName.hpp>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <list>

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
    memset(LocalUserName,0,NAME_SIZE*MAX_TTYS);
    memset(LocalLoginName,0,NAME_SIZE*MAX_TTYS);
    memset(LocalLoginType,0,MAX_TTYS);
    memset(ActiveLocalUserName,0,NAME_SIZE);
    memset(ActiveLocalLoginName,0,NAME_SIZE);
    ActiveLocalLoginType = ' ';
    memset(RemoteUserName,0,NAME_SIZE*MAX_TTYS);
    memset(RemoteLoginName,0,NAME_SIZE*MAX_TTYS);
    memset(RemoteLoginType,' ',MAX_TTYS);
    NumOfLocalUsers = 0;
    NumOfRemoteUsers = 0;
    NumOfRDSKRemoteUsers = 0;
    NumOfVNCRemoteUsers = 0;
    TimeStamp = 0;
    PowerDown = 0;
    CheckSum = 0;
}

//------------------------------------------------------------------------------

void CStatDatagram::Clear(void)
{
    memset(Header,0,HEADER_SIZE);
    memset(NodeName,0,NAME_SIZE);
    memset(LocalUserName,0,NAME_SIZE*MAX_TTYS);
    memset(LocalLoginName,0,NAME_SIZE*MAX_TTYS);
    memset(LocalLoginType,0,MAX_TTYS);
    memset(ActiveLocalUserName,0,NAME_SIZE);
    memset(ActiveLocalLoginName,0,NAME_SIZE);
    ActiveLocalLoginType = ' ';
    memset(RemoteUserName,0,NAME_SIZE*MAX_TTYS);
    memset(RemoteLoginName,0,NAME_SIZE*MAX_TTYS);
    memset(RemoteLoginType,' ',MAX_TTYS);

    NumOfLocalUsers = 0;
    NumOfRemoteUsers = 0;
    NumOfRDSKRemoteUsers = 0;
    NumOfVNCRemoteUsers = 0;

    TimeStamp = 0;
    PowerDown = 0;
    CheckSum = 0;
}

//------------------------------------------------------------------------------

void CStatDatagram::SetDatagram(bool powerdown)
{
    memset(Header,0,HEADER_SIZE);
    memset(NodeName,0,NAME_SIZE);
    memset(LocalUserName,0,NAME_SIZE*MAX_TTYS);
    memset(LocalLoginName,0,NAME_SIZE*MAX_TTYS);
    memset(LocalLoginType,0,MAX_TTYS);
    memset(ActiveLocalUserName,0,NAME_SIZE);
    memset(ActiveLocalLoginName,0,NAME_SIZE);
    ActiveLocalLoginType = ' ';
    memset(RemoteUserName,0,NAME_SIZE*MAX_TTYS);
    memset(RemoteLoginName,0,NAME_SIZE*MAX_TTYS);
    memset(RemoteLoginType,' ',MAX_TTYS);

    NumOfLocalUsers = 0;
    NumOfRemoteUsers = 0;
    NumOfRDSKRemoteUsers = 0;
    NumOfVNCRemoteUsers = 0;

    TimeStamp = 0;
    PowerDown = 0;
    CheckSum = 0;

    memcpy(Header,"STAT",4);

    gethostname(NodeName,NAME_SIZE-1);

    // get list of local sessions
    std::list<CUserSession> sessions;

    FILE* p_sf = popen("/bin/loginctl --no-legend --no-pager list-sessions","r");

    CSmallString buffer;
    while( (p_sf != NULL) && (buffer.ReadLineFromFile(p_sf,true,true)) ) {
        stringstream str(buffer.GetBuffer());
        string sid;
        string uid;
        string user;
        string seat;
        string tty;
        str >> sid >> uid >> user >> seat >> tty;
        if( ! (sid .empty() || uid.empty() || user.empty()) ){
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
                CUserSession ses;
                ses.SessionID = sid;
                strncpy(ses.UserName,name,NAME_SIZE-1);
                strncpy(ses.LoginName,login,NAME_SIZE-1);
                sessions.push_back(ses);
            }
        }
    }
    if( p_sf ) pclose(p_sf);

    // load active tty
    CSmallString active_tty;
    active_tty = "TTY=";
    FILE* p_tty0 = fopen("/sys/class/tty/tty0/active","r");
    if( p_tty0 ){
        active_tty.ReadLineFromFile(p_tty0,true,false);
        fclose(p_tty0);
    }

    // deep seesion investigation
    std::list<CUserSession>::iterator   it = sessions.begin();
    std::list<CUserSession>::iterator   ie = sessions.end();

    while(it != ie){
        CUserSession ses = *it;
        CSmallString cmd;
        bool remote = false;
        bool x11 = false;
        bool wayland = false;
        bool loctty = false;
        bool vnc = false;
        bool rdsk = false;

        cmd << "/bin/loginctl show-session " << ses.SessionID;
        FILE* p_sf = popen(cmd,"r");
        if( p_sf ){
            CSmallString buffer;
            while( buffer.ReadLineFromFile(p_sf,true,true) ){
                if( buffer.FindSubString("Type=x11") != -1 ) x11 = true;
                if( buffer.FindSubString("Type=wayland") != -1 ) wayland = true;
                if( buffer.FindSubString("Service=tigervnc") != -1 ) rdsk = true;
                if( buffer.FindSubString("Remote=yes") != -1 ) remote = true;
                if( buffer.FindSubString(active_tty) != -1 ) loctty = true;
            }
            pclose(p_sf);
        }

        cmd = NULL;
        cmd << "/bin/loginctl session-status " << ses.SessionID;
        p_sf = popen(cmd,"r");
        if( p_sf ){
            CSmallString buffer;
            buffer.SetLength(BUFFER_LEN);   // +1 for \0 is added internally
            while( buffer.ReadLineFromFile(p_sf,true,true) ){
                if( buffer.FindSubString("Xvnc") != -1 ) vnc = true;
            }
            pclose(p_sf);
        }

        if( (remote == false) && ((x11 == true) || (wayland == true)) ){
            if( NumOfLocalUsers < MAX_TTYS ){
                strncpy(LocalUserName[NumOfLocalUsers],ses.UserName,NAME_SIZE-1);
                strncpy(LocalLoginName[NumOfLocalUsers],ses.LoginName,NAME_SIZE-1);
                if( wayland == true ){
                    LocalLoginType[NumOfRemoteUsers] = 'W';
                } else {
                    LocalLoginType[NumOfRemoteUsers] = 'X';
                }
                if( loctty ){
                    ActiveLocalLoginType = LocalLoginType[NumOfRemoteUsers];
                    strncpy(ActiveLocalUserName,ses.UserName,NAME_SIZE-1);
                    strncpy(ActiveLocalLoginName,ses.LoginName,NAME_SIZE-1);
                }
                NumOfLocalUsers++;
            }
        }
        if( remote == true ){
            if( NumOfRemoteUsers < MAX_TTYS ){
                strncpy(RemoteUserName[NumOfRemoteUsers],ses.UserName,NAME_SIZE-1);
                strncpy(RemoteLoginName[NumOfRemoteUsers],ses.LoginName,NAME_SIZE-1);
                if( rdsk == true ){
                    RemoteLoginType[NumOfRemoteUsers] = 'R';
                } else if( vnc == true ){
                    RemoteLoginType[NumOfRemoteUsers] = 'V';
                } else {
                    RemoteLoginType[NumOfRemoteUsers] = 'S';
                }
                NumOfRemoteUsers++;
                if( rdsk == true ) NumOfRDSKRemoteUsers++;
                if( vnc == true )  NumOfVNCRemoteUsers++;
            }
        }

        it++;
    }

    PowerDown = 0;
    if( powerdown ){
        PowerDown = 1;
        // clear :-(
        memset(LocalUserName,0,NAME_SIZE*MAX_TTYS);
        memset(LocalLoginName,0,NAME_SIZE*MAX_TTYS);
        memset(LocalLoginType,0,MAX_TTYS);
        memset(ActiveLocalUserName,0,NAME_SIZE);
        memset(ActiveLocalLoginName,0,NAME_SIZE);
        ActiveLocalLoginType = ' ';
        memset(RemoteUserName,0,NAME_SIZE*MAX_TTYS);
        memset(RemoteLoginName,0,NAME_SIZE*MAX_TTYS);
        memset(RemoteLoginType,' ',MAX_TTYS);

        NumOfLocalUsers = 0;
        NumOfRemoteUsers = 0;
        NumOfRDSKRemoteUsers = 0;
        NumOfVNCRemoteUsers = 0;
    }

// finalize -----------------------
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
            CheckSum += (unsigned char)LocalUserName[k][i];
            CheckSum += (unsigned char)LocalLoginName[k][i];
        }
    }
    for(size_t i=0; i < MAX_TTYS; i++){
        CheckSum += (unsigned char)LocalLoginType[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        CheckSum += (unsigned char)ActiveLocalUserName[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        CheckSum += (unsigned char)ActiveLocalLoginName[i];
    }

    CheckSum += (unsigned char)ActiveLocalLoginType;

    for(size_t k=0; k < MAX_TTYS; k++){
        for(size_t i=0; i < NAME_SIZE; i++){
            CheckSum += (unsigned char)RemoteUserName[k][i];
            CheckSum += (unsigned char)RemoteLoginName[k][i];
        }
    }
    for(size_t i=0; i < MAX_TTYS; i++){
        CheckSum += (unsigned char)RemoteLoginType[i];
    }

    CheckSum += NumOfLocalUsers;
    CheckSum += NumOfRemoteUsers;
    CheckSum += NumOfRDSKRemoteUsers;
    CheckSum += NumOfVNCRemoteUsers;
    CheckSum += PowerDown;
    CheckSum += TimeStamp;
}

//------------------------------------------------------------------------------

void CStatDatagram::SetNodeName(const CSmallString& name)
{
    strncpy(NodeName,name,NAME_SIZE-1);
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
            checksum += (unsigned char)LocalUserName[k][i];
            checksum += (unsigned char)LocalLoginName[k][i];
        }
    }
    for(size_t i=0; i < MAX_TTYS; i++){
        checksum += (unsigned char)LocalLoginType[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        checksum += (unsigned char)ActiveLocalUserName[i];
    }
    for(size_t i=0; i < NAME_SIZE; i++){
        checksum += (unsigned char)ActiveLocalLoginName[i];
    }

    checksum += (unsigned char)ActiveLocalLoginType;

    for(size_t k=0; k < MAX_TTYS; k++){
        for(size_t i=0; i < NAME_SIZE; i++){
            checksum += (unsigned char)RemoteUserName[k][i];
            checksum += (unsigned char)RemoteLoginName[k][i];
        }
    }

    for(size_t i=0; i < MAX_TTYS; i++){
        checksum += (unsigned char)RemoteLoginType[i];
    }

    checksum += NumOfLocalUsers;
    checksum += NumOfRemoteUsers;
    checksum += NumOfRDSKRemoteUsers;
    checksum += NumOfVNCRemoteUsers;
    checksum += PowerDown;
    checksum += TimeStamp;

    return( checksum == CheckSum );
}

//------------------------------------------------------------------------------

void CStatDatagram::PrintInfo(std::ostream& vout)
{
    vout << "Node  = " << GetNodeName() << endl;
    vout << "User  = " << GetLocalUserName() << endl;
    vout << "Login = " << GetLocalLoginName() << endl;
    vout << "#L    = " << NumOfLocalUsers << endl;
    vout << "#R    = " << NumOfRemoteUsers << endl;
    vout << "#RDSK = " << NumOfRDSKRemoteUsers << endl;
    vout << "#VNC  = " << NumOfVNCRemoteUsers << endl;
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetNodeName(void)
{
    NodeName[NAME_SIZE-1] = '\0';
    return(NodeName);
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetLocalUserName(void)
{
    return(ActiveLocalUserName);
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetLocalLoginName(void)
{
    return(ActiveLocalLoginName);
}

//------------------------------------------------------------------------------

char CStatDatagram::GetLocalLoginType(void)
{
    return(ActiveLocalLoginType);
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetLocalUserName(int id)
{
    if( (id >= 0) && (id < MAX_TTYS) ){
        LocalUserName[id][NAME_SIZE-1] = '\0';
        return(LocalUserName[id]);
    }
    return("");
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetLocalLoginName(int id)
{
    if( (id >= 0) && (id < MAX_TTYS) ){
        LocalLoginName[id][NAME_SIZE-1] = '\0';
        return(LocalLoginName[id]);
    }
    return("");
}

//------------------------------------------------------------------------------

char CStatDatagram::GetLocalLoginType(int id)
{
    if( (id >= 0) && (id < MAX_TTYS) ){
        return(LocalLoginType[id]);
    }
    return(' ');
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetRemoteUserName(int id)
{
    if( (id >= 0) && (id < MAX_TTYS) ){
        RemoteUserName[id][NAME_SIZE-1] = '\0';
        return(RemoteUserName[id]);
    }
    return("");
}

//------------------------------------------------------------------------------

CSmallString CStatDatagram::GetRemoteLoginName(int id)
{
    if( (id >= 0) && (id < MAX_TTYS) ){
        RemoteLoginName[id][NAME_SIZE-1] = '\0';
        return(RemoteLoginName[id]);
    }
    return("");
}

//------------------------------------------------------------------------------

char CStatDatagram::GetRemoteLoginType(int id)
{
    if( (id >= 0) && (id < MAX_TTYS) ){
        return(RemoteLoginType[id]);
    }
    return(' ');
}

//------------------------------------------------------------------------------

int CStatDatagram::GetTimeStamp(void)
{
    return(TimeStamp);
}

//------------------------------------------------------------------------------

bool CStatDatagram::IsDown(void)
{
    return(PowerDown == 1);
}

//------------------------------------------------------------------------------

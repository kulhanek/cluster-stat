#ifndef WolfStatClientOptionsH
#define WolfStatClientOptionsH
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

#include <SimpleOptions.hpp>
#include <StatMainHeader.hpp>

//------------------------------------------------------------------------------

class CStatClientOptions : public CSimpleOptions {
public:
    // constructor - tune option setup
    CStatClientOptions(void);

    // program name and description -----------------------------------------------
    CSO_PROG_NAME_BEGIN
    "wolf-stat-client"
    CSO_PROG_NAME_END

    CSO_PROG_DESC_BEGIN
    "The program notifies the server about logged users."
    CSO_PROG_DESC_END

    CSO_PROG_VERS_BEGIN
    StatBuildVersion
    CSO_PROG_VERS_END

    // list of all options and arguments ------------------------------------------
    CSO_LIST_BEGIN
    // arguments ------------------------------
    CSO_ARG(CSmallString,ServerName)
    // options ------------------------------
    CSO_OPT(int,Port)
    CSO_OPT(int,Interval)
    CSO_OPT(bool,Help)
    CSO_OPT(bool,Version)
    CSO_OPT(bool,Verbose)
    CSO_LIST_END

    CSO_MAP_BEGIN
    // description of arguments ---------------------------------------------------
    CSO_MAP_ARG(CSmallString,                   /* argument type */
                ServerName,                          /* argument name */
                NULL,                           /* default value */
                true,                           /* is argument mandatory */
                "servername",                        /* parametr name */
                "IP address or name of server, which collects statististics\n")   /* argument description */
    // description of options -----------------------------------------------------
    CSO_MAP_OPT(int,                            /* option type */
                Port,                           /* option name */
                32598,                          /* default value */
                false,                          /* is option mandatory */
                'p',                           /* short option name */
                "port",                      /* long option name */
                "PORT",                           /* parametr name */
                "port number for communication")   /* option description */
    //----------------------------------------------------------------------
    CSO_MAP_OPT(int,                            /* option type */
                Interval,                           /* option name */
                15,                          /* default value */
                false,                          /* is option mandatory */
                'i',                           /* short option name */
                "interval",                      /* long option name */
                "TIME",                           /* parametr name */
                "delay between regular node status updates (zero value means one shot update)")   /* option description */
    //----------------------------------------------------------------------
    CSO_MAP_OPT(bool,                           /* option type */
                Verbose,                        /* option name */
                false,                          /* default value */
                false,                          /* is option mandatory */
                'v',                           /* short option name */
                "verbose",                      /* long option name */
                NULL,                           /* parametr name */
                "increase output verbosity")   /* option description */
    //----------------------------------------------------------------------
    CSO_MAP_OPT(bool,                           /* option type */
                Version,                        /* option name */
                false,                          /* default value */
                false,                          /* is option mandatory */
                '\0',                           /* short option name */
                "version",                      /* long option name */
                NULL,                           /* parametr name */
                "output version information and exit")   /* option description */
    //----------------------------------------------------------------------
    CSO_MAP_OPT(bool,                           /* option type */
                Help,                        /* option name */
                false,                          /* default value */
                false,                          /* is option mandatory */
                'h',                           /* short option name */
                "help",                      /* long option name */
                NULL,                           /* parametr name */
                "display this help and exit")   /* option description */
    CSO_MAP_END

    // final operation with options ------------------------------------------------
private:
    virtual int CheckOptions(void);
    virtual int FinalizeOptions(void);
    virtual int CheckArguments(void);
};

//------------------------------------------------------------------------------

#endif

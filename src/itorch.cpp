#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "lsUtil.h"
#include "irods_buffer_encryption.hpp"
#include <string>
#include <iostream>

#pragma clang diagnostic ignored "-Wunused-function"
#include "experimental_plugin_framework.hpp"

void usage()
{
    std::cout << "ITORCH\n";
}

int main(int argc, char* argv[])
{
    signal( SIGPIPE, SIG_IGN );

    if (NULL != argv && NULL != argv[0]) {
        /* set SP_OPTION to argv[0] so it can be passed to server */
        char child[MAX_NAME_LEN], parent[MAX_NAME_LEN];
        *child = '\0';
        splitPathByKey(argv[0], parent, MAX_NAME_LEN, child, MAX_NAME_LEN, '/');
        if (*child != '\0') {
            mySetenvStr(SP_OPTION, child);
        }
    }

    rodsArguments_t rods_args;
    auto opts = "hArlLvt:VZ";
    auto err = parseCmdLineOpt( argc, argv, opts, 1, &rods_args );
    if ( err < 0 ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }

    if ( rods_args.help == True ) {
        usage();
        exit( 0 );
    }

    rodsEnv env;
    err = getRodsEnv( &env );
    if (err < 0) {
        rodsLogError(LOG_ERROR, err, "main: getRodsEnv error. ");
        exit(1);
    }

    rErrMsg_t err_msg;
    auto comm = rcConnect(
                    env.rodsHost,
                    env.rodsPort,
                    env.rodsUserName,
                    env.rodsZone,
                    0, &err_msg );
    if (nullptr == comm) {
        exit(2);
    }

    irods::pack_entry_table& pk_tbl = irods::get_pack_table();
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    if (strcmp(env.rodsUserName, PUBLIC_USER_NAME ) != 0) {
        err = clientLogin( comm );
        if ( err != 0 ) {
            rcDisconnect( comm );
            exit( 7 );
        }
    }

    namespace ie = irods::experimental;

    json req {
        {ie::commands::request, ie::endpoints::operation},
        {ie::constants::plugin, "recursive_remove"},
        {"logical_path", "/tempZone/home/rods/coll0"}};

    auto rep = invoke(comm, req);


    std::string x{};
#if 1
    uint32_t prog{};
    while(ie::states::complete != x &&
          ie::states::failed   != x) {

        usleep(100);

        json req{};

        if(prog > 30) {
            req = {
                {ie::commands::request,  ie::endpoints::command},
                {ie::constants::command, ie::commands::cancel}, // shut it down
                {ie::constants::plugin,  "recursive_remove"}};
        }
        else {

            req = {
                {ie::commands::request,  ie::endpoints::command},
                {ie::constants::command, ie::commands::progress},
                {ie::constants::plugin,  "recursive_remove"}};
        }

        auto rep = invoke(comm, req);

        try {
            x = get<std::string>(ie::constants::status, rep);
            prog = std::stol(x);
        }
        catch(...) {
        }

        std::cout << "status: " << x << "\n";
    }
#else

        try {
            x = get<std::string>(ie::constants::status, rep);
        }
        catch(...) {
        }
        std::cout << "status: " << x << "\n";
#endif


    rcDisconnect(comm);

    return 0;
} // main

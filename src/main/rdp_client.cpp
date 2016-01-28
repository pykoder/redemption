/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Product name: redemption, a FLOSS RDP proxy
 * Copyright (C) Wallix 2015
 *
 * Standalone RDP client main program
 *
 */

#include <iostream>
#include <string>

#define LOGPRINT
#include "log.hpp"

#include "client_front.hpp"
#include "wait_obj.hpp"
#include "mod_api.hpp"
#include "redirection_info.hpp"
#include "rdp/rdp_params.hpp"
#include "genrandom.hpp"
#include "rdp/rdp.hpp"
#include "program_options/program_options.hpp"

namespace po = program_options;
//using namespace std;


void run_mod(mod_api & mod, ClientFront & front, wait_obj & front_event, SocketTransport * st_mod, SocketTransport * st_front);

int main(int argc, char** argv)
{
    const char * copyright_notice =
        "\n"
        "Standalone RDP Client.\n"
        "Copyright (C) Wallix 2010-2015.\n"
        "\n"
        ;

    RedirectionInfo redir_info;
    int verbose = 0;
    std::string target_device = "10.10.47.205";
    int target_port = 3389;
    int nbretry = 3;
    int retry_delai_ms = 1000;

    std::string username = "administrateur";
    std::string password = "SecureLinux";
    ClientInfo client_info;

    client_info.width = 800;
    client_info.height = 600;
    client_info.bpp = 32;

    /* Program options */
    po::options_description desc({
        {'h', "help","produce help message"},
        {'t', "target-device", &target_device, "target device"},
        {'u', "username", &username, "username"},
        {'p', "password", &password, "password"},
        {"verbose", &verbose, "verbose"},
    });

    auto options = po::parse_command_line(argc, argv, desc);

    if (options.count("help") > 0) {
        std::cout << copyright_notice;
        std::cout << "Usage: rdptproxy [options]\n\n";
        std::cout << desc << std::endl;
        exit(0);
    }


    int client_sck = ip_connect(target_device.c_str(), target_port, nbretry, retry_delai_ms, verbose);
    SocketTransport mod_trans( "RDP Client", client_sck, target_device.c_str(), target_port, verbose, nullptr);


    ClientFront front(mod_trans, client_info, verbose);
    ModRDPParams mod_rdp_params( username.c_str()
                               , password.c_str()
                               , target_device.c_str()
                               , "0.0.0.0"   // client ip is silenced
                               , /*front.keymap.key_flags*/ 0
                               , verbose);

    wait_obj front_event;

    /* Random */
    LCGRandom gen(0);

    /* mod_api */
    mod_rdp mod( mod_trans, front, client_info, redir_info, gen, mod_rdp_params);

    run_mod(mod, front, front_event, &mod_trans, nullptr);

    return 0;
}



void run_mod(mod_api & mod, ClientFront & front, wait_obj & front_event, SocketTransport * st_mod, SocketTransport * st_front) {
    struct      timeval time_mark = { 0, 50000 };
    bool        run_session       = true;

    while (run_session) {
        try {
            unsigned max = 0;
            fd_set   rfds;
            fd_set   wfds;

            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            struct timeval timeout = time_mark;

            add_to_fd_set(mod.get_event(), st_mod, rfds, max, timeout);

            if (is_set(mod.get_event(), st_mod, rfds)) {
                timeout.tv_sec  = 2;
                timeout.tv_usec = 0;
            }

            int num = select(max + 1, &rfds, &wfds, nullptr, &timeout);

            LOG(LOG_INFO, "RDP CLIENT :: select num = %d\n", num);

            if (num < 0) {
                if (errno == EINTR) {
                    continue;
                }

                LOG(LOG_INFO, "RDP CLIENT :: errno = %d\n", errno);
                break;
            }

            if (is_set(mod.get_event(), st_mod, rfds)) {
                LOG(LOG_INFO, "RDP CLIENT :: draw_event");
                mod.draw_event(time(nullptr));

                try {
                    front.process_incoming_data();
                }
                catch (...) {
                   // run_session = false;
                    continue;
                };
            }

        } catch (Error & e) {
            LOG(LOG_ERR, "RDP CLIENT :: Exception raised = %d!\n", e.id);
            run_session = false;
        };
    }   // while (run_session)
    return;
}

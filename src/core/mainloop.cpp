/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Product name: redemption, a FLOSS RDP proxy
  Copyright (C) Wallix 2010 - 2012
  Author(s): Christophe Grosjean, Raphael Zhou

  Main loop
*/

#include <unistd.h>
#include <signal.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "config.hpp"
#include "mainloop.hpp"
#include "log.hpp"
#include "listen.hpp"
#include "session_server.hpp"
#include "parse_ip_conntrack.hpp"

#include "config.hpp"

/*****************************************************************************/
#ifndef IN_IDE_PARSER
[[noreturn]]
#endif
void shutdown(int sig)
{
    LOG(LOG_INFO, "shutting down : signal %d pid=%d\n", sig, getpid());
    exit(1);
}

/*****************************************************************************/
void sigpipe(int sig)
{
    LOG(LOG_INFO, "got SIGPIPE(%d) : ignoring\n", sig);
}

/*****************************************************************************/
//void sigsegv(int sig)
//{
//    LOG(LOG_INFO, "Ignoring SIGSEGV : signal %d \n", sig);
//}

void sighup(int sig)
{
    LOG(LOG_INFO, "Ignoring SIGHUP : signal %d \n", sig);
}

//void sigchld(int sig)
//{
//    // triggered when a child close. For now we will just ignore this signal
//    // because there is no child termination management yet.
//    // When there will be child management code, we will have to setup
//    // some communication protocol to discuss with childs.
//    LOG(LOG_INFO, "Ignoring SIGCHLD : signal %d pid %d\n", sig, getpid());
//}

void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGBUS);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigaddset(&sa.sa_mask, SIGALRM);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGUSR2);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGSEGV, &sa, nullptr);

    sa.sa_handler = SIG_DFL;
    sigaction(SIGBUS, &sa, nullptr);

    sa.sa_handler = shutdown;
    sigaction(SIGTERM, &sa, nullptr);

    sa.sa_handler = sighup;
    sigaction(SIGHUP, &sa, nullptr);

    sa.sa_handler = shutdown;
    sigaction(SIGINT, &sa, nullptr);

    sa.sa_handler = sigpipe;
    sigaction(SIGPIPE, &sa, nullptr);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);

    sa.sa_handler = SIG_DFL;
    sigaction(SIGALRM, &sa, nullptr);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &sa, nullptr);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGUSR2, &sa, nullptr);
}

//void reset_signals(void)
//{
//    struct sigaction sa;
//
//    sa.sa_flags = 0;
//    sa.sa_handler = SIG_DFL;
//
//    sigemptyset(&sa.sa_mask);
//    sigaddset(&sa.sa_mask, SIGSEGV);
//    sigaddset(&sa.sa_mask, SIGBUS);
//    sigaddset(&sa.sa_mask, SIGTERM);
//    sigaddset(&sa.sa_mask, SIGHUP);
//    sigaddset(&sa.sa_mask, SIGINT);
//    sigaddset(&sa.sa_mask, SIGPIPE);
//    sigaddset(&sa.sa_mask, SIGCHLD);
//    sigaddset(&sa.sa_mask, SIGALRM);
//    sigaddset(&sa.sa_mask, SIGUSR1);
//    sigaddset(&sa.sa_mask, SIGUSR2);
//
//    sigaction(SIGSEGV, &sa, nullptr);
//    sigaction(SIGBUS, &sa, nullptr);
//    sigaction(SIGTERM, &sa, nullptr);
//    sigaction(SIGHUP, &sa, nullptr);
//    sigaction(SIGINT, &sa, nullptr);
//    sigaction(SIGPIPE, &sa, nullptr);
//    sigaction(SIGCHLD, &sa, nullptr);
//    sigaction(SIGALRM, &sa, nullptr);
//    sigaction(SIGUSR1, &sa, nullptr);
//    sigaction(SIGUSR2, &sa, nullptr);
//}

void redemption_new_session(char const * config_filename)
{
    char text[256];
    char source_ip[256];
    int target_port = 0;
    char real_target_ip[256];

    union
    {
        struct sockaddr s;
        struct sockaddr_storage ss;
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
    } u;
    int sock_len = sizeof(u);

    Inifile ini;
    { ConfigurationLoader cfg_loader(ini, config_filename); }

    init_signals();
    snprintf(text, 255, "redemption_%8.8x_main_term", unsigned(getpid()));

    getpeername(0, &u.s, (socklen_t *)&sock_len);
    strcpy(source_ip, inet_ntoa(u.s4.sin_addr));

    union
    {
        struct sockaddr s;
        struct sockaddr_storage ss;
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
    } localAddress;
    socklen_t addressLength = sizeof(localAddress);

    int sck = 0;
    if (-1 == getsockname(sck, &localAddress.s, &addressLength)){
        LOG(LOG_INFO, "getsockname failed error=%s", strerror(errno));
        _exit(1);
    }

    target_port = localAddress.s4.sin_port;
    strcpy(real_target_ip, inet_ntoa(localAddress.s4.sin_addr));

    if (ini.get<cfg::globals::enable_ip_transparent>()) {
        const int source_port = 0;
        char target_ip[256];
        strcpy(target_ip, inet_ntoa(localAddress.s4.sin_addr));
        int fd = open("/proc/net/ip_conntrack", O_RDONLY);
        // source and dest are inverted because we get the information we want from reply path rule
        int res = parse_ip_conntrack(fd, target_ip, source_ip, target_port, source_port, real_target_ip, sizeof(real_target_ip));
        if (res){
            LOG(LOG_WARNING, "Failed to get transparent proxy target from ip_conntrack");
        }
        close(fd);
    }


    ini.set_acl<cfg::globals::host>(source_ip);
    ini.set_acl<cfg::globals::target>(real_target_ip);

    if (ini.get<cfg::debug::session>()){
        LOG(LOG_INFO, "Setting new session socket to %d\n", sck);
    }

    int nodelay = 1;
    if (0 == setsockopt(sck, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay))){
        Session session(sck, ini);

        if (ini.get<cfg::debug::session>()){
            LOG(LOG_INFO, "Session::end of Session(%u)", sck);
        }

        shutdown(sck, 2);
        close(sck);
    }
    else {
        LOG(LOG_INFO, "Failed to set socket TCP_NODELAY option on client socket");
    }

}

void redemption_main_loop(Inifile & ini, unsigned uid, unsigned gid, std::string config_filename)
{
    init_signals();

    SessionServer ss(uid, gid, std::move(config_filename), ini.get<cfg::debug::config>() == Inifile::ENABLE_DEBUG_CONFIG);
    //    Inifile ini(CFG_PATH "/" RDPPROXY_INI);
    uint32_t s_addr = inet_addr(ini.get<cfg::globals::listen_address>());
    if (s_addr == INADDR_NONE) { s_addr = INADDR_ANY; }
    int port = ini.get<cfg::globals::port>();
    Listen listener( ss
                     , s_addr
                     , port
                     , false                              /* exit on timeout       */
                     , 60                                 /* timeout sec           */
                     , ini.get<cfg::globals::enable_ip_transparent>()
                     );
    listener.run();
}

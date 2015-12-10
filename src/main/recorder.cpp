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
 * Copyright (C) Wallix 2010-2013
 * Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen
 * Raphaël Zhou
 *
 * recorder main program
 *
 */

#include <signal.h>

//#define LOGPRINT
#include "log.hpp"

bool program_requested_to_shutdown = false;

void shutdown(int sig)
{
    LOG(LOG_INFO, "shutting down : signal %d pid=%d\n", sig, getpid());

    program_requested_to_shutdown = true;
}

void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);

    sa.sa_handler = shutdown;
    sigaction(SIGTERM, &sa, nullptr);
}

extern "C" int recorder_main(int argc, char** argv, bool & program_requested_to_shutdown);

int main(int argc, char** argv)
{
    init_signals();

    return recorder_main(argc, argv, program_requested_to_shutdown);
}

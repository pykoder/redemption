/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean

   Transport layer abstraction
*/

#ifndef _REDEMPTION_CORE_OUTMETATRANSPORT_HPP_
#define _REDEMPTION_CORE_OUTMETATRANSPORT_HPP_

#include "transport.hpp"
#include "error.hpp"

#include "rio/rio.h"

class OutmetaTransport : public Transport {
public:
    timeval now;
    char meta_path[1024];
    char path[1024];

    RIO * rio;
    SQ * seq;

    OutmetaTransport(const char * path, const char * basename, timeval now, uint16_t width, uint16_t height, unsigned verbose = 0)
    : now(now)
    , rio(NULL)
    , seq(NULL)
    {
        RIO_ERROR status = RIO_ERROR_OK;
        char filename[1024];
        sprintf(filename, "%s-%06u", basename, getpid());
        char header1[1024];
        sprintf(header1, "%u %u", width, height);
        this->rio = rio_new_outmeta(&status, &this->seq, path, filename, ".mwrm", header1, "0", "", &now);
        if (status < 0){
            throw Error(ERR_TRANSPORT_WRITE_FAILED, errno);
        }
    }

    ~OutmetaTransport()
    {
        rio_delete(this->rio);
    }

    using Transport::send;
    virtual void send(const char * const buffer, size_t len) throw (Error) {
        ssize_t res = rio_send(this->rio, buffer, len);
        if (res < 0){
            throw Error(ERR_TRANSPORT_WRITE_FAILED, errno);
        }
    }

    using Transport::recv;
    virtual void recv(char**, size_t) throw (Error)
    {  
        LOG(LOG_INFO, "OutmetaTransport used for recv");
        throw Error(ERR_TRANSPORT_OUTPUT_ONLY_USED_FOR_SEND, 0);
    }

    virtual void timestamp(timeval now)
    {
        sq_timestamp(this->seq, &now);
        Transport::timestamp(now);
    }

    virtual bool next()
    {
        sq_next(this->seq);
        return Transport::next();
    }
};

#endif

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
Copyright (C) Wallix 2010-2019
Author(s): Jonathan Poelen
*/

#include "utils/stream.hpp"
#include "mod/rdp/rdp_verbose.hpp"
#include "redjs/js_table_id.hpp"

#include <memory>

class Callback;

namespace redjs
{

struct Clipboard
{
    Clipboard(JsTableId id, RDPVerbose verbose);
    ~Clipboard();

    void receive(InStream chunk, int flags);
    void set_cb(Callback* cb);

    void send_request_format(uint32_t id);

private:
    class D;
    std::unique_ptr<D> d;
};

} // namespace redjs

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
  Copyright (C) Wallix 2010
  Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat,
             Raphael Zhou, Meng Tan

  Manage Modules Life cycle : creation, destruction and chaining
  find out the next module to run from context reading
*/

#pragma once

#include "acl/mm_api.hpp"
#include "utils/rect.hpp"

class Inifile;
class SessionReactor;

namespace GCC::UserData
{
    class CSMonitor;
}

class MMIni
{
public:

    ModWrapper & get_mod_wrapper() 
    {
        return mod_wrapper;
    }

    mod_api* get_mod()
    {
        return this->mod_wrapper.get_mod();
    }

    [[nodiscard]] mod_api const* get_mod() const
    {
        return this->mod_wrapper.get_mod();
    }

public:
    bool last_module{false};
    bool connected{false};

    void invoke_close_box(
        bool enable_close_box,
        const char * auth_error_message, BackEvent_t & signal,
        AuthApi & authentifier, ReportMessageApi & report_message);

    bool is_connected() {
        return this->connected;
    }
    bool is_up_and_running() {
        return this->mod_wrapper.is_up_and_running();
    }

    [[nodiscard]] rdp_api* get_rdp_api() const { return nullptr; }

protected:
    Inifile& ini;
    SessionReactor& session_reactor;
    ModWrapper mod_wrapper;

public:
    explicit MMIni(SessionReactor& session_reactor, Inifile & ini_)
    : ini(ini_)
    , session_reactor(session_reactor)
    {}

    void remove_mod() {}

    void new_mod(ModuleIndex target_module, AuthApi & /*unused*/, ReportMessageApi & /*unused*/);

    ModuleIndex next_module();

    void check_module()
    {
        if (this->ini.get<cfg::context::forcemodule>() && !this->is_connected()) {
            this->session_reactor.set_next_event(BACK_EVENT_NEXT);
            this->ini.set<cfg::context::forcemodule>(false);
            // Do not send back the value to sesman.
        }
    }
};

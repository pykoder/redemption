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

//#define LOGPRINT
#include "version.hpp"

#include "capture.hpp"

#include "apps/app_recorder.hpp"
#include "program_options/program_options.hpp"

namespace po = program_options;

extern "C" {

int recorder_main(int argc, char** argv, bool & program_requested_to_shutdown)
{
    struct CaptureMaker {
        Capture capture;

        CaptureMaker( const timeval & now, uint16_t width, uint16_t height, int order_bpp, int capture_bpp
                    , const char * path, const char * basename, const char * /*extension*/
                    , Inifile & ini, bool /*clear*/, uint32_t /*verbose*/)
        : capture( now, width, height, order_bpp
                 , capture_bpp
                 , path, path, ini.get<cfg::video::hash_path>(), basename
                 , false, false, nullptr, ini, true)
        {}
    };
    return app_recorder<CaptureMaker>(
        argc, argv
      , "ReDemPtion RECorder " VERSION ": An RDP movie converter.\n"
        "Copyright (C) Wallix 2010-2015.\n"
        "Christophe Grosjean, Jonathan Poelen and Raphael Zhou."
      , program_requested_to_shutdown
      , [](po::options_description const &){}
      , [](Inifile const & ini, po::variables_map const &, std::string const & output_filename) -> int {
            if (   output_filename.length()
                && !(
                    bool(ini.get<cfg::video::capture_flags>()
                        & (configs::CaptureFlags::png | configs::CaptureFlags::wrm)
                    ) | ini.get<cfg::globals::capture_chunk>()
                )
            ) {
                std::cerr << "Missing target format : need --png or --wrm\n" << std::endl;
                return -1;
            }
            return 0;
      }
//      , [](cfg::crypto::key0::type const &, cfg::crypto::key1::type const &) { return 0; }
      , [](Inifile const &) { return false; }/*has_extra_capture*/
    );
}

}
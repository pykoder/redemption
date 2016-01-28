/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Copyright (C) Wallix 2015

    Front object (client), used to communicate with RDP server
*/

#ifndef _CLIENT_FRONT_HPP_
#define _CLIENT_FRONT_HPP_

#include "log.hpp"

#include "front_api.hpp"
#include "RDP/x224.hpp"
#include "stream.hpp"
#include "transport.hpp"
#include "client_info.hpp"
#include "channel_list.hpp"
#include "RDP/RDPDrawable.hpp"
#include "socket_transport.hpp"
#include "socket_transport_utility.hpp"
#include "RDP/orders/RDPOrdersSecondaryBrushCache.hpp"
#include "RDP/orders/RDPOrdersSecondaryColorCache.hpp"

class ClientFront : public FrontAPI {

    private:
    Transport & trans;

    public:
    uint32_t verbose;
    ClientInfo &info;
    uint8_t                     mod_bpp;
    BGRPalette                  mod_palette;
    RDPDrawable gd;
    CHANNELS::ChannelDefArray   cl;
    CryptContext decrypt;

    ClientFront(Transport & trans, ClientInfo & info, uint32_t verbose)
    : FrontAPI(false, false), trans(trans), verbose(verbose), info(info), mod_bpp(info.bpp), mod_palette(BGRPalette::no_init()), gd(info.width, info.height, 24)
       {
        if (this->mod_bpp == 8) {
            this->mod_palette = BGRPalette::classic_332();
        }

        this->verbose = verbose;

        memset(this->decrypt.key, 0, 16);
        memset(this->decrypt.update_key, 0, 16);
        this->decrypt.encryptionMethod = 1;  /* todo : see what encryption level is supported : low, medium high, 0 1 2*/

        SSL_library_init();
    }

    void process_incoming_data()
    {
        if (this->verbose & 8) {
            LOG(LOG_INFO, "ClientFront::process_incoming_data()\n\n\n");
        }
        constexpr std::size_t array_size = 65536;
        uint8_t array[array_size];
        uint8_t * end = array;
        X224::RecvFactory fx224(this->trans, &end, array_size, true);
        InStream stream(array, end - array);
        if (fx224.fast_path) {
            FastPath::ClientInputEventPDU_Recv cfpie(stream, this->decrypt, array);
            if (this->verbose & 8) {
                LOG(LOG_INFO, "ClientFront::fast_path supported");
                LOG(LOG_INFO, "cfpie.numEvents = %u", cfpie.numEvents);
            }
            for (uint8_t i = 0; i < cfpie.numEvents; i++) {
                if (!cfpie.payload.in_check_rem(1)) {
                    LOG(LOG_WARNING, "Front::Received fast-path PUD, remains=%zu", cfpie.payload.in_remain());
                    throw Error(ERR_RDP_DATA_TRUNCATED);
                }

                uint8_t byte = cfpie.payload.in_uint8();
                uint8_t eventCode  = (byte & 0xE0) >> 5;

                switch (eventCode) {
                    case FastPath::FASTPATH_INPUT_EVENT_SCANCODE:
                    {
                        FastPath::KeyboardEvent_Recv ke(cfpie.payload, byte);

                        if (this->verbose & 4) {
                            LOG(LOG_INFO, "Front::Received fast-path PUD, scancode keyboardFlags=0x%X, keyCode=0x%X",
                                ke.spKeyboardFlags, ke.keyCode);
                        }
                    }
                    break;

                    case FastPath::FASTPATH_INPUT_EVENT_MOUSE:
                    {
                        FastPath::MouseEvent_Recv me(cfpie.payload, byte);
                        if (this->verbose & 4) {
                            LOG(LOG_INFO, "ClientFront::Received fast-path PUD, mouse pointerFlags=0x%X, xPos=0x%X, yPos=0x%X",
                                me.pointerFlags, me.xPos, me.yPos);
                        }
                    }
                    break;

                    default:
                        LOG(LOG_INFO, "Front::Received unexpected fast-path PUD, eventCode = %u", eventCode);
                        throw Error(ERR_RDP_FASTPATH);
                }

                if (this->verbose & 4) {
                    LOG(LOG_INFO, "Front::Received fast-path PUD done");
                }

                if (cfpie.payload.in_remain() != 0) {
                    LOG(LOG_WARNING, "Front::Received fast-path PUD, remains=%zu", cfpie.payload.in_remain());
                }
            }
        }
        else{
            if (this->verbose & 8) {
                LOG(LOG_INFO, "ClientFront::fast_path not supported");
            }
        }

    }

    void flush() override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "flush()");
            LOG(LOG_INFO, "========================================\n");
        }
    }

    void draw(const RDPOpaqueRect & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPOpaqueRect new_cmd24 = cmd;
        new_cmd24.color = color_decode_opaquerect(cmd.color, this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPScrBlt & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip);
    }

    void draw(const RDPDestBlt & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip);
    }

    void draw(const RDPMultiDstBlt & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip);
    }

    void draw(const RDPMultiOpaqueRect & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip);
    }

    void draw(const RDP::RDPMultiPatBlt & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip);
    }

    void draw(const RDP::RDPMultiScrBlt & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip);
    }

    void draw(const RDPPatBlt & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPPatBlt new_cmd24 = cmd;
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
        new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPMemBlt & cmd, const Rect & clip, const Bitmap & bitmap) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip, bitmap);
    }

    void draw(const RDPMem3Blt & cmd, const Rect & clip, const Bitmap & bitmap) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(cmd, clip, bitmap);
    }

    void draw(const RDPLineTo & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPLineTo new_cmd24 = cmd;
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
        new_cmd24.pen.color  = color_decode_opaquerect(cmd.pen.color,  this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPGlyphIndex & cmd, const Rect & clip, const GlyphCache * gly_cache) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPGlyphIndex new_cmd24 = cmd;
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
        new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip, gly_cache);
    }

    void draw(const RDPPolygonSC & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPPolygonSC new_cmd24 = cmd;
        new_cmd24.BrushColor  = color_decode_opaquerect(cmd.BrushColor,  this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPPolygonCB & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPPolygonCB new_cmd24 = cmd;
        new_cmd24.foreColor  = color_decode_opaquerect(cmd.foreColor,  this->mod_bpp, this->mod_palette);
        new_cmd24.backColor  = color_decode_opaquerect(cmd.backColor,  this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPPolyline & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPPolyline new_cmd24 = cmd;
        new_cmd24.PenColor  = color_decode_opaquerect(cmd.PenColor,  this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPEllipseSC & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPEllipseSC new_cmd24 = cmd;
        new_cmd24.color = color_decode_opaquerect(cmd.color, this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPEllipseCB & cmd, const Rect & clip) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        RDPEllipseCB new_cmd24 = cmd;
        new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
        this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPColCache   & cmd) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }
    }

    void draw(const RDPBrushCache & cmd) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            cmd.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }
    }

    void draw(const RDP::FrameMarker & order) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            order.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(order);
    }

    void draw(const RDP::RAIL::NewOrExistingWindow & order) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            order.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(order);
    }

    void draw(const RDP::RAIL::WindowIcon & order) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            order.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(order);
    }

    void draw(const RDP::RAIL::CachedIcon & order) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            order.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(order);
    }

    void draw(const RDP::RAIL::DeletedWindow & order) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            order.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(order);
    }

    void draw(const RDPBitmapData & bitmap_data, const uint8_t * data,
        size_t size, const Bitmap & bmp) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            bitmap_data.log(LOG_INFO, "ClientFront");
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.draw(bitmap_data, data, size, bmp);

    }

    void send_global_palette() override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "send_global_palette()");
            LOG(LOG_INFO, "========================================\n");
        }
    }

    int server_resize(int width, int height, int bpp) override {
        this->mod_bpp = bpp;
        this->info.bpp = bpp;
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "server_resize(width=%d, height=%d, bpp=%d", width, height, bpp);
            LOG(LOG_INFO, "========================================\n");
        }
        return 1;
    }

    void server_set_pointer(const Pointer & cursor) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "server_set_pointer");
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.server_set_pointer(cursor);
    }

    void begin_update() override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "begin_update");
            LOG(LOG_INFO, "========================================\n");
        }
    }

    void end_update() override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "end_update");
            LOG(LOG_INFO, "========================================\n");
        }
    }

    void server_draw_text( Font const & font, int16_t x, int16_t y, const char * text, uint32_t fgcolor
                         , uint32_t bgcolor, const Rect & clip) {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "server_draw_text %s", text);
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.server_draw_text(
            font, x, y, text,
            color_decode_opaquerect(fgcolor, this->mod_bpp, this->mod_palette),
            color_decode_opaquerect(bgcolor, this->mod_bpp, this->mod_palette),
            clip
        );
    }

    void text_metrics(Font const & font, const char* text, int& width, int& height) {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "text_metrics");
            LOG(LOG_INFO, "========================================\n");
        }

        this->gd.text_metrics(font, text, width, height);
    }

    // reutiliser le FakeFront
    // creer un main calqué sur celui de transparent.cpp et reussir a lancer un mod_rdp
    const CHANNELS::ChannelDefArray & get_channel_list(void) const override { return cl; }

    void send_to_channel( const CHANNELS::ChannelDef & channel, const uint8_t * data, std::size_t length
                        , std::size_t chunk_size, int flags) override {
        if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- ClientFront ------------------");
            LOG(LOG_INFO, "send_to_channel");
            LOG(LOG_INFO, "========================================\n");
        }
    }

};

#endif

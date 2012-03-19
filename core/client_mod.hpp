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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   module object. Some kind of interface between core and modules

*/
#if !defined(__CLIENT_MOD_HPP__)
#define __CLIENT_MOD_HPP__

#include <string.h>
#include <stdio.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <locale.h>

#include "client_info.hpp"
#include "font.hpp"
#include "cache.hpp"
#include "front.hpp"
#include "mainloop.hpp"
#include "bmpcache.hpp"
#include "wait_obj.hpp"
#include "keymap.hpp"
#include "callback.hpp"
#include "modcontext.hpp"

enum BackEvent_t {
    BACK_EVENT_NONE = 0,
    BACK_EVENT_1,
    BACK_EVENT_2,
    BACK_EVENT_3,
    BACK_EVENT_STOP = 4,
    BACK_EVENT_5,
    BACK_EVENT_REFRESH,
};

struct client_mod : public Callback {
    FrontAPI & front;
    Rect clip;
    RDPPen pen;

    bool clipboard_enable;
    bool pointer_displayed;
    int sck;
    char ip_source[256];
    int rdp_compression;
    int bitmap_cache_persist_enable;
    uint8_t socket;

    wait_obj * event;
    BackEvent_t signal;

    client_mod(FrontAPI & front)
        : front(front),
          signal(BACK_EVENT_NONE)    {
        this->pointer_displayed = false;
        this->front.mod_palette_setted = false;
        this->front.mod_bpp = 24;
        this->front.palette_sent = false;
        for (size_t i = 0; i < 6 ; i++){
            this->front.palette_memblt_sent[i] = false;
        }
        init_palette332(this->front.palette332);
    }

    virtual ~client_mod()
    {
    }


    // draw_event is run when mod socket received some data (drawing order),
    // these order could also be auto-generated, say to comply to some refresh.

    // draw_event returns not 0 (return status) when the module finished
    // (connection to remote or internal server closed)
    // and returns 0 as long as the connection with server is still active.
    virtual BackEvent_t draw_event(void) = 0;

    virtual void refresh_context(ModContext & context)
    {
        return; // used when context changed to avoid creating a new module
    }

    void server_resize(int width, int height, int bpp)
    {
        int client_info_width = this->front.get_front_width();
        int client_info_height = this->front.get_front_height();
        int client_info_bpp = this->front.get_front_bpp();
        int client_info_build = this->front.get_front_build();

        if (client_info_width != width
        || client_info_height != height
        || client_info_bpp != bpp) {
            /* older client can't resize */
            if (client_info_build <= 419) {
                LOG(LOG_ERR, "Resizing is not available on older RDP clients");
                return;
            }
            this->front.palette_sent = false;
            for (size_t i = 0; i < 6 ; i++){
                this->front.palette_memblt_sent[i] = false;
            }
            LOG(LOG_INFO, "// Resizing client to : %d x %d x %d\n", width, height, bpp);

            this->front.set_front_resolution(width, height, bpp);
            TODO("This warns modules that the resolution has changed, it is only used in internal modules.")
            this->front_resize();
//            this->front.reset();
        }
    }

    void front_resize()
    {
    }

    int server_is_term()
    {
        return g_is_term();
    }

    void server_add_char(int font, int character,
                    int offset, int baseline,
                    int width, int height, const uint8_t* data)
    {
        struct FontChar fi(offset, baseline, width, height, 0);
        memcpy(fi.data, data, fi.datasize());
        this->glyph_cache(fi, font, character);
    }

    void send_to_front_channel(const char * const mod_channel_name, uint8_t* data, size_t length, size_t chunk_size, int flags)
    {
        const McsChannelItem * front_channel = this->front.get_channel_list().get(mod_channel_name);
        if (front_channel){
            this->front.send_to_channel(*front_channel, data, length, chunk_size, flags);
        }
    }

    bool get_pointer_displayed() {
        return this->pointer_displayed;
    }

    void set_pointer_display() {
        this->pointer_displayed = true;
    }

    void text_metrics(const char * text, int & width, int & height){
        height = 0;
        width = 0;
        if (text) {
            size_t len = mbstowcs(0, text, 0);
            wchar_t wstr[len + 2];
            mbstowcs(wstr, text, len + 1);
            for (size_t index = 0; index < len; index++) {
                FontChar *font_item = this->front.font.font_items[wstr[index]];
                width += font_item->incby;
                height = std::max(height, font_item->height);
            }
        }
    }

    void glyph_cache(const FontChar & font_char, int font_index, int char_index)
    {
        RDPGlyphCache cmd(font_index, 1, char_index, font_char.offset, font_char.baseline, font_char.width, font_char.height, font_char.data);
        this->front.orders->draw(cmd);
    }

    void server_set_pen(int style, int width)
    {
        this->pen.style = style;
        this->pen.width = width;
    }


    void server_set_pointer(int x, int y, uint8_t* data, uint8_t* mask)
    {
        int cache_idx = 0;
        switch (this->front.cache.add_pointer(data, mask, x, y, cache_idx)){
        case POINTER_TO_SEND:
            this->front.send_pointer(cache_idx, data, mask, x, y);
        break;
        default:
        case POINTER_ALLREADY_SENT:
            this->front.set_pointer(cache_idx);
        break;
        }
    }

    int get_front_bpp() const {
        return this->front.get_front_bpp();
    }

    int get_front_width() const {
        return this->front.get_front_width();
    }

    int get_front_height() const {
        return this->front.get_front_height();
    }

     const Rect get_front_rect(){
        return Rect(0, 0, this->get_front_width(), get_front_height());
    }

    virtual void rdp_input_invalidate(const Rect & r) = 0;
    virtual void rdp_input_synchronize(uint32_t time, uint16_t device_flags, int16_t param1, int16_t param2) = 0;
    virtual void rdp_input_scancode(long param1, long param2, long param3, long param4, const Keymap * keymap, const key_info* ki) = 0;
    virtual void rdp_input_mouse(int device_flags, int x, int y, const Keymap * keymap) = 0;

};

#endif

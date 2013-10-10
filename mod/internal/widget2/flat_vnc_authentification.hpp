/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou
 */

#if !defined(REDEMPTION_MOD_INTERNAL_WIDGET2_FLAT_VNC_AUTHENTIFICATION_HPP)
#define REDEMPTION_MOD_INTERNAL_WIDGET2_FLAT_VNC_AUTHENTIFICATION_HPP

#include "edit.hpp"
#include "label.hpp"
#include "password.hpp"
#include "multiline.hpp"
#include "image.hpp"
#include "composite.hpp"
#include "flat_button.hpp"
#include "translation.hpp"

class FlatVNCAuthentification : public WidgetParent
{
public:
    WidgetLabel password_label;
    WidgetPassword  password_edit;

    int fgcolor;
    int bgcolor;

    FlatVNCAuthentification(DrawApi& drawable, uint16_t width, uint16_t height, Widget2 & parent,
              NotifyApi* notifier, const char* caption,
              int group_id,
              const char * password,
              int fgcolor, int bgcolor,
              const char * label_text_password)
        : WidgetParent(drawable, Rect(0, 0, width, height), parent, notifier)
        , password_label(drawable, 0, 0, *this, NULL, label_text_password, true, -13, fgcolor, bgcolor)
        , password_edit(drawable, 0, 0, 400, *this, this, password, -14, BLACK, WHITE, -1u, 1, 1)
        , fgcolor(fgcolor)
        , bgcolor(bgcolor)
    {
        this->impl = new CompositeTable;
        this->add_widget(&this->password_edit);
        this->add_widget(&this->password_label);


        this->password_edit.set_flat(true);
        // Center bloc positionning
        // Login and Password boxes
        int cbloc_w = this->password_label.rect.cx + this->password_edit.rect.cx + 10;
        int cbloc_h = std::max(this->password_label.rect.cy,
                               this->password_edit.rect.cy);


        int x_cbloc = (width  - cbloc_w)/2;
        int y_cbloc = (height - cbloc_h)/2;

        this->password_label.set_xy(x_cbloc, y_cbloc);
        this->password_edit.set_xy(x_cbloc + this->password_label.rect.cx + 10, y_cbloc);

        this->password_label.rect.y += (this->password_edit.cy() - this->password_label.cy()) / 2;
    }

    virtual ~FlatVNCAuthentification()
    {
        this->clear();
    }

    virtual void draw(const Rect& clip)
    {
        this->impl->draw(clip);
        this->draw_inner_free(clip.intersect(this->rect), this->bgcolor);
    }

    virtual void draw_inner_free(const Rect& clip, int bg_color) {
        Region region;
        region.rects.push_back(clip);

        this->impl->draw_inner_free(clip, bg_color, region);

        for (std::size_t i = 0, size = region.rects.size(); i < size; ++i) {
            this->drawable.draw(RDPOpaqueRect(region.rects[i], bg_color), region.rects[i]);
        }
    }

    virtual void notify(Widget2* widget, NotifyApi::notify_event_t event)
    {
        if ((widget == &this->password_edit)
             && event == NOTIFY_SUBMIT) {
            this->send_notify(NOTIFY_SUBMIT);
        }
    }

    virtual void rdp_input_scancode(long int param1, long int param2, long int param3, long int param4, Keymap2* keymap)
    {
        if (keymap->nb_kevent_available() > 0){
            switch (keymap->top_kevent()){
            case Keymap2::KEVENT_ESC:
                keymap->get_kevent();
                this->send_notify(NOTIFY_CANCEL);
                break;
            default:
                WidgetParent::rdp_input_scancode(param1, param2, param3, param4, keymap);
                break;
            }
        }
    }
};

#endif

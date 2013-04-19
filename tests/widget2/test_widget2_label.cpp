/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen
 */

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestWidgetLabel
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL
#include "log.hpp"

#include "internal/widget2/label.hpp"
// #include "internal/widget2/widget_composite.hpp"
#include "png.hpp"
#include "ssl_calls.hpp"
#include "RDP/RDPDrawable.hpp"
#include "check_sig.hpp"

struct TestDraw : ModApi
{
    RDPDrawable gd;
    Font font;

    TestDraw(uint16_t w, uint16_t h)
    : gd(w, h, true)
    , font(FIXTURES_PATH "/dejavu-sans-10.fv1")
    {}

    virtual void draw(const RDPOpaqueRect& cmd, const Rect& rect)
    {
        this->gd.draw(cmd, rect);
    }

    virtual void draw(const RDPScrBlt&, const Rect&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPDestBlt&, const Rect&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPPatBlt&, const Rect&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPMemBlt& cmd, const Rect& rect, const Bitmap& bmp)
    {
        this->gd.draw(cmd, rect, bmp);
    }

    virtual void draw(const RDPLineTo&, const Rect&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPGlyphIndex&, const Rect&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPBrushCache&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPColCache&)
    {
        BOOST_CHECK(false);
    }

    virtual void draw(const RDPGlyphCache&)
    {
        BOOST_CHECK(false);
    }

    virtual void begin_update()
    {}

    virtual void end_update()
    {}

    virtual void server_draw_text(int x, int y, const char* text, uint32_t fgcolor, const Rect& clip)
    {
        this->gd.server_draw_text(x, y, text, fgcolor, clip, this->font);
    }

    virtual void text_metrics(const char* text, int& width, int& height)
    {
        height = 0;
        width = 0;
        uint32_t uni[256];
        size_t len_uni = UTF8toUnicode(reinterpret_cast<const uint8_t *>(text), uni, sizeof(uni)/sizeof(uni[0]));
        if (len_uni){
            for (size_t index = 0; index < len_uni; index++) {
                FontChar *font_item = this->gd.get_font(this->font, uni[index]);
                width += font_item->width + 2;
                height = std::max(height, font_item->height);
            }
            width -= 2;
        }
    }

    void save_to_png(const char * filename)
    {
        std::FILE * file = fopen(filename, "w+");
        dump_png24(file, this->gd.drawable.data, this->gd.drawable.width,
                   this->gd.drawable.height, this->gd.drawable.rowsize);
        fclose(file);
    }
};

BOOST_AUTO_TEST_CASE(TraceWidgetLabel)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget at position 0,0 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 0;
    int16_t y = 0;
    int xtext = 4;
    int ytext = 1;

    TODO("I believe users of this widget may wish to control text position and behavior inside rectangle"
         "ie: text may be centered, aligned left, aligned right, or even upside down, etc"
         "these possibilities (and others) are supported in RDPGlyphIndex")
    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test1", auto_resize, id, bg_color, fg_color, xtext, ytext);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

     //drawable.save_to_png("/tmp/label.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\xfc\x7b\x97\x0f\xcf\x88\x82\xb1\x20\x11"
        "\xe4\xd3\x51\x4e\xf0\xce\xce\xeb\xc0\x58")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel2)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 10,100 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 10;
    int16_t y = 100;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test2", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png("/tmp/label2.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\x6c\x91\x76\x52\x46\xb1\x93\x8a\xf1\xf0"
        "\x79\x78\xa8\x50\x30\x0a\x1c\xd8\x08\x98")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel3)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position -10,500 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = -10;
    int16_t y = 500;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test3", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png("/tmp/label3.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\x8d\xf1\x81\xc0\xd5\x5b\x9b\x49\x37\x46"
        "\x2a\x66\x6a\x16\xe9\x3d\xeb\x5e\x09\xac")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel4)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 770,500 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 770;
    int16_t y = 500;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test4", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png("/tmp/label4.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\x97\xa2\x50\x07\x36\xfc\x5c\x4b\x77\x7c"
        "\xd4\x84\xb4\xc6\x87\x0c\xed\xa3\x4b\x82")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel5)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position -20,-7 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = -20;
    int16_t y = -7;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test5", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png("/tmp/label5.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\x23\x28\x0a\xb4\xe8\xde\x21\xb0\xa7\xbd"
        "\x84\x97\x82\xcb\xb0\xe9\x72\x5d\x3b\xe9")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel6)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 760,-7 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 760;
    int16_t y = -7;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test6", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png("/tmp/label6.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\x81\x40\x2e\xae\x9d\x5b\x38\x94\x16\x5d"
        "\x86\x19\xac\x2e\xd4\x2c\xe7\xd6\x53\xdb")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabelClip)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 760,-7 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 760;
    int16_t y = -7;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test6", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at position 780,-7 and of size 120x20. After clip the size is of 20x13
    wlabel.rdp_input_invalidate(Rect(20 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

     //drawable.save_to_png("/tmp/label7.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\x51\x61\x10\xd2\xe0\xcc\xef\xe0\x15\xc6"
        "\x3c\x2a\x14\x62\xa0\xfb\xd5\xc6\x2c\x1d")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabelClip2)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 10,7 in it's parent context
    Widget2* parent = NULL;
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 0;
    int16_t y = 0;

    WidgetLabel wlabel(&drawable, x, y, parent, notifier, "test6", auto_resize, id, bg_color, fg_color);

    // ask to widget to redraw at position 30,12 and of size 30x10.
    wlabel.rdp_input_invalidate(Rect(20 + wlabel.dx(),
                                     5 + wlabel.dy(),
                                     30,
                                     10));

    //drawable.save_to_png("/tmp/label8.png");

    char message[1024];
    if (!check_sig(drawable.gd.drawable, message,
        "\xa2\x85\x04\x67\x88\x91\x62\x24\xf9\x6d"
        "\x79\x33\x7d\x21\x34\xe5\x67\x2c\xdb\x86")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

// BOOST_AUTO_TEST_CASE(TraceWidgetLabelEvent)
// {
//     struct WidgetReceiveEvent : public Widget {
//         Widget2* sender;
//         NotifyApi::notify_event_t event;
//
//         WidgetReceiveEvent()
//         : Widget(NULL, Rect(), NULL, NULL)
//         {}
//
//         virtual void draw(const Rect&)
//         {}
//
//         virtual void notify(Widget2* sender, NotifyApi::notify_event_t event,
//                             unsigned long, unsigned long)
//         {
//             this->sender = sender;
//             this->event = event;
//         }
//     } widget_for_receive_event;
//
//     Widget2* parent = &widget_for_receive_event;
//     ModApi * drawable = NULL;
//     NotifyApi * notifier = NULL;
//     bool auto_resize = false;
//     int16_t x = 0;
//     int16_t y = 0;
//
//     WidgetLabel wlabel(drawable, x, y, parent, notifier, "", auto_resize);
//
//     wlabel.rdp_input_mouse(CLIC_BUTTON1_UP, 0, 0, 0);
//     BOOST_CHECK(widget_for_receive_event.sender == &wlabel);
//     BOOST_CHECK(widget_for_receive_event.event == CLIC_BUTTON1_UP);
//     widget_for_receive_event.sender = 0;
//     widget_for_receive_event.event = 0;
//     wlabel.rdp_input_mouse(CLIC_BUTTON1_DOWN, 0, 0, 0);
//     BOOST_CHECK(widget_for_receive_event.sender == &wlabel);
//     BOOST_CHECK(widget_for_receive_event.event == CLIC_BUTTON1_DOWN);
//     widget_for_receive_event.sender = 0;
//     widget_for_receive_event.event = 0;
//     wlabel.rdp_input_mouse(KEYUP, 0, 0, 0);
//     BOOST_CHECK(widget_for_receive_event.sender == &wlabel);
//     BOOST_CHECK(widget_for_receive_event.event == KEYUP);
//     widget_for_receive_event.sender = 0;
//     widget_for_receive_event.event = 0;
//     wlabel.rdp_input_mouse(KEYDOWN, 0, 0, 0);
//     BOOST_CHECK(widget_for_receive_event.sender == &wlabel);
//     BOOST_CHECK(widget_for_receive_event.event == KEYDOWN);
// }

// BOOST_AUTO_TEST_CASE(TraceWidgetLabelAndComposite)
// {
//     TestDraw drawable(800, 600);
//
//     // WidgetLabel is a label widget of size 256x125 at position 0,0 in it's parent context
//     Widget2* parent = NULL;
//     NotifyApi * notifier = NULL;
//
//     WidgetComposite wcomposite(&drawable, Rect(0,0,800,600), parent, notifier);
//
//     WidgetLabel wlabel1(&drawable, 0,0, &wcomposite, notifier,
//                         "abababab", true, 0, YELLOW, BLACK);
//     WidgetLabel wlabel2(&drawable, 0,100, &wcomposite, notifier,
//                         "ggghdgh", true, 0, WHITE, RED);
//     WidgetLabel wlabel3(&drawable, 100,100, &wcomposite, notifier,
//                         "lldlslql", true, 0, BLUE, RED);
//     WidgetLabel wlabel4(&drawable, 300,300, &wcomposite, notifier,
//                         "LLLLMLLM", true, 0, PINK, DARK_GREEN);
//     WidgetLabel wlabel5(&drawable, 700,-10, &wcomposite, notifier,
//                         "dsdsdjdjs", true, 0, LIGHT_GREEN, DARK_BLUE);
//     WidgetLabel wlabel6(&drawable, -10,550, &wcomposite, notifier,
//                         "xxwwp", true, 0, DARK_GREY, PALE_GREEN);
//
//     wcomposite.child_list.push_back(&wlabel1);
//     wcomposite.child_list.push_back(&wlabel2);
//     wcomposite.child_list.push_back(&wlabel3);
//     wcomposite.child_list.push_back(&wlabel4);
//     wcomposite.child_list.push_back(&wlabel5);
//     wcomposite.child_list.push_back(&wlabel6);
//
//     // ask to widget to redraw at position 100,25 and of size 100x100.
//     wcomposite.rdp_input_invalidate(Rect(100, 25, 100, 100));
//
//     //drawable.save_to_png("/tmp/label9.png");
//
//     char message[1024];
//     if (!check_sig(drawable.gd.drawable, message,
//         "\x3f\x02\x08\xad\xbd\xd8\xf2\xc7\x1b\xf8"
//         "\x32\x58\x67\x66\x5d\xdb\xe5\x75\xe4\xda")){
//         BOOST_CHECK_MESSAGE(false, message);
//     }
//
//     // ask to widget to redraw at it's current position
//     wcomposite.rdp_input_invalidate(Rect(0, 0, wcomposite.cx(), wcomposite.cy()));
//
//     //drawable.save_to_png("/tmp/label10.png");
//
//     if (!check_sig(drawable.gd.drawable, message,
//         "\x85\x0a\x9c\x09\x57\xd9\x99\x52\xed\xa8"
//         "\x25\x71\x91\x6c\xf4\xf4\x21\x9a\xe5\x1a")){
//         BOOST_CHECK_MESSAGE(false, message);
//     }
// }

TODO("the entry point exists in module: it's rdp_input_invalidate"
     "je just have to change received values to widget messages")

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
   Author(s): Christophe Grosjean, Javier Caverni, Dominique Lafages
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Colors object. Contains generic colors
*/

#ifndef _REDEMPTION_UTILS_COLORS_HPP_
#define _REDEMPTION_UTILS_COLORS_HPP_

#include <stdint.h>
//#include <assert.h>
#include <stdlib.h>
#include <cstddef>
#include "log.hpp"

typedef uint32_t BGRColor;

inline BGRColor RGBtoBGR(const BGRColor & c){
    return ((c << 16) & 0xFF0000)|(c & 0x00FF00)|((c>>16) & 0x0000FF);
}

struct BGRPalette
{
    BGRPalette() = delete;

    struct no_init {};
    explicit BGRPalette(no_init) noexcept
    {}

    explicit BGRPalette(std::nullptr_t) noexcept
    : palette{0}
    {}

    explicit BGRPalette(uint8_t const * palette_data) noexcept
    { this->set_data(palette_data); }

    static const BGRPalette & classic_332_rgb() noexcept
    {
        static const BGRPalette palette([](int c) { return RGBtoBGR(c); }, 1);
        return palette;
    }

    static const BGRPalette & classic_332() noexcept
    {
        static const BGRPalette palette([](int c) { return static_cast<BGRColor>(c); }, 1);
        return palette;
    }

    BGRColor operator[](std::size_t i) const noexcept
    { return this->palette[i]; }

    void set_color(std::size_t i, BGRColor c) noexcept
    { this->palette[i] = c; }

    void set_data(uint8_t const * palette_data)
    { memcpy(this->palette, palette_data, sizeof(this->palette)); }

    const char * data() const noexcept
    { return reinterpret_cast<char const*>(this->palette); }

    static constexpr std::size_t data_size() noexcept
    { return sizeof(palette); }

private:
    BGRColor palette[256];

    template<class Transform>
    /*constexpr*/ BGRPalette(Transform trans, int) noexcept
    {
        /* rgb332 palette */
        for (int bindex = 0; bindex < 4; bindex++) {
            for (int gindex = 0; gindex < 8; gindex++) {
                for (int rindex = 0; rindex < 8; rindex++) {
                    this->palette[(rindex << 5) | (gindex << 2) | bindex] =
                    trans(
                    // r1 r2 r2 r1 r2 r3 r1 r2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
                        (((rindex<<5)|(rindex<<2)|(rindex>>1))<<16)
                    // 0 0 0 0 0 0 0 0 g1 g2 g3 g1 g2 g3 g1 g2 0 0 0 0 0 0 0 0
                    | (((gindex<<5)|(gindex<<2)|(gindex>>1))<< 8)
                    // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 b1 b2 b1 b2 b1 b2 b1 b2
                    | ((bindex<<6)|(bindex<<4)|(bindex<<2)|(bindex)));
                }
            }
        }
    }
};

// typedef BGRColor BGRPalette[256];

// Those are in BGR
enum {
    BLACK                     = 0x000000,
    GREY                      = 0xc0c0c0,
    MEDIUM_GREY               = 0xa0a0a0,
    DARK_GREY                 = 0x8c8a8c,
    ANTHRACITE                = 0x808080,
    WHITE                     = 0xffffff,

    BLUE                      = 0xff0000,
    DARK_BLUE                 = 0x7f0000,
    CYAN                      = 0xffff00,
    DARK_BLUE_WIN             = 0x602000,
    DARK_BLUE_BIS             = 0x601f08,
    MEDIUM_BLUE               = 0xC47244,
    PALE_BLUE                 = 0xf6ece9,
    LIGHT_BLUE                = 0xebd5cf,
    WINBLUE                   = 0x9C4D00,

    RED                       = 0x0000ff,
    DARK_RED                  = 0x221CAD,
    MEDIUM_RED                = 0x302DB7,
    PINK                      = 0xff00ff,

    GREEN                     = 0x00ff00,
    WABGREEN                  = 0x2BBE91,
    WABGREEN_BIS              = 0x08ff7b,
    DARK_WABGREEN             = 0x91BE2B,
    INV_DARK_WABGREEN         = 0x2BBE91,
    DARK_GREEN                = 0x499F74,
    INV_DARK_GREEN            = 0x749F49,
    LIGHT_GREEN               = 0x90ffe0,
    INV_LIGHT_GREEN           = 0x8EE537,
    PALE_GREEN                = 0xE1FAF0,
    INV_PALE_GREEN            = 0xF0FAE1,
    MEDIUM_GREEN              = 0xACE4C8,
    INV_MEDIUM_GREEN          = 0xC8E4AC,

    YELLOW                    = 0x00ffff,
    LIGHT_YELLOW              = 0x9fffff,

    ORANGE                    = 0x1580DD,
    LIGHT_ORANGE              = 0x64BFFF,
    PALE_ORANGE               = 0x9AD5FF,
    BROWN                     = 0x006AC5
};


inline unsigned color_from_cstr(const char * str) {
    unsigned res = 0;

    if (0 == strcasecmp("BLACK", str))                  { res = BLACK; }
    else if (0 == strcasecmp("GREY", str))              { res = GREY; }
    else if (0 == strcasecmp("DARK_GREY", str))         { res = DARK_GREY; }
    else if (0 == strcasecmp("ANTHRACITE", str))        { res = ANTHRACITE; }
    else if (0 == strcasecmp("BLUE", str))              { res = BLUE; }
    else if (0 == strcasecmp("DARK_BLUE", str))         { res = DARK_BLUE; }
    else if (0 == strcasecmp("WHITE", str))             { res = WHITE; }
    else if (0 == strcasecmp("RED", str))               { res = RED; }
    else if (0 == strcasecmp("PINK", str))              { res = PINK; }
    else if (0 == strcasecmp("GREEN", str))             { res = GREEN; }
    else if (0 == strcasecmp("YELLOW", str))            { res = YELLOW; }
    else if (0 == strcasecmp("LIGHT_YELLOW", str))      { res = LIGHT_YELLOW; }
    else if (0 == strcasecmp("CYAN", str))              { res = CYAN; }
    else if (0 == strcasecmp("WABGREEN", str))          { res = WABGREEN; }
    else if (0 == strcasecmp("WABGREEN_BIS", str))      { res = WABGREEN_BIS; }
    else if (0 == strcasecmp("DARK_WABGREEN", str))     { res = DARK_WABGREEN; }
    else if (0 == strcasecmp("INV_DARK_WABGREEN", str)) { res = INV_DARK_WABGREEN; }
    else if (0 == strcasecmp("DARK_GREEN", str))        { res = DARK_GREEN; }
    else if (0 == strcasecmp("INV_DARK_GREEN", str))    { res = INV_DARK_GREEN; }
    else if (0 == strcasecmp("LIGHT_GREEN", str))       { res = LIGHT_GREEN; }
    else if (0 == strcasecmp("INV_LIGHT_GREEN", str))   { res = INV_LIGHT_GREEN; }
    else if (0 == strcasecmp("PALE_GREEN", str))        { res = PALE_GREEN; }
    else if (0 == strcasecmp("INV_PALE_GREEN", str))    { res = INV_PALE_GREEN; }
    else if (0 == strcasecmp("MEDIUM_GREEN", str))      { res = MEDIUM_GREEN; }
    else if (0 == strcasecmp("INV_MEDIUM_GREEN", str))  { res = INV_MEDIUM_GREEN; }
    else if (0 == strcasecmp("DARK_BLUE_WIN", str))     { res = DARK_BLUE_WIN; }
    else if (0 == strcasecmp("DARK_BLUE_BIS", str))     { res = DARK_BLUE_BIS; }
    else if (0 == strcasecmp("MEDIUM_BLUE", str))       { res = MEDIUM_BLUE; }
    else if (0 == strcasecmp("PALE_BLUE", str))         { res = PALE_BLUE; }
    else if (0 == strcasecmp("LIGHT_BLUE", str))        { res = LIGHT_BLUE; }
    else if (0 == strcasecmp("WINBLUE", str))           { res = WINBLUE; }
    else if (0 == strcasecmp("ORANGE", str))            { res = ORANGE; }
    else if (0 == strcasecmp("DARK_RED", str))          { res = DARK_RED; }
    else if (0 == strcasecmp("BROWN", str))             { res = BROWN; }
    else if (0 == strcasecmp("LIGHT_ORANGE", str))      { res = LIGHT_ORANGE; }
    else if (0 == strcasecmp("PALE_ORANGE", str))       { res = PALE_ORANGE; }
    else if (0 == strcasecmp("MEDIUM_RED", str))        { res = MEDIUM_RED; }
    else if ((*str == '0') && (*(str + 1) == 'x')){
        res = RGBtoBGR(strtol(str + 2, nullptr, 16));
    }
    else { res = RGBtoBGR(atol(str)); }

    return res;
}

// colorN (variable): an index into the current palette or an RGB triplet
//                    value; the actual interpretation depends on the color
//                    depth of the bitmap data.
// +-------------+------------+------------------------------------------------+
// | Color depth | Field size |                Meaning                         |
// +-------------+------------+------------------------------------------------+
// |       8 bpp |     1 byte |     Index into the current color palette.      |
// +-------------+------------+------------------------------------------------+
// |      15 bpp |    2 bytes | RGB color triplet expressed in 5-5-5 format    |
// |             |            | (5 bits for red, 5 bits for green, and 5 bits  |
// |             |            | for blue).                                     |
// +-------------+------------+------------------------------------------------+
// |      16 bpp |    2 bytes | RGB color triplet expressed in 5-6-5 format    |
// |             |            | (5 bits for red, 6 bits for green, and 5 bits  |
// |             |            | for blue).                                     |
// +-------------+------------+------------------------------------------------+
// |    24 bpp   |    3 bytes |     RGB color triplet (1 byte per component).  |
// +-------------+------------+------------------------------------------------+

inline BGRColor color_decode(const BGRColor c, const uint8_t in_bpp, const BGRPalette & palette){
    switch (in_bpp){
    case 1:
    {
        uint32_t res = 0;
        if (c) res = 0x00FFFFFF;
        return res;
    }
    break;
    case 8:
        return palette[static_cast<uint8_t>(c)] & 0xFFFFFF;
    case 15:
    {
        // r1 r2 r3 r4 r5 g1 g2 g3 g4 g5 b1 b2 b3 b4 b5
        const BGRColor r = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7); // r1 r2 r3 r4 r5 r1 r2 r3
        const BGRColor g = ((c >> 2) & 0xf8) | ((c >>  7) & 0x7); // g1 g2 g3 g4 g5 g1 g2 g3
        const BGRColor b = ((c << 3) & 0xf8) | ((c >>  2) & 0x7); // b1 b2 b3 b4 b5 b1 b2 b3
        return (r << 16) | (g << 8) | b;
    }
    break;
    case 16:
    {
        // r1 r2 r3 r4 r5 g1 g2 g3 g4 g5 g6 b1 b2 b3 b4 b5
        const BGRColor r = ((c >> 8) & 0xf8) | ((c >> 13) & 0x7); // r1 r2 r3 r4 r5 r6 r7 r8
        const BGRColor g = ((c >> 3) & 0xfc) | ((c >>  9) & 0x3); // g1 g2 g3 g4 g5 g6 g1 g2
        const BGRColor b = ((c << 3) & 0xf8) | ((c >>  2) & 0x7); // b1 b2 b3 b4 b5 b1 b2 b3
        return (r << 16) | (g << 8) | b;
    }
    case 32:
    case 24:
      return c & 0xFFFFFF;
    default:
        LOG(LOG_ERR, "in_bpp = %d", in_bpp);
        exit(0);
    }
    return 0;
}


inline BGRColor color_decode_opaquerect(const BGRColor c, const uint8_t in_bpp, const BGRPalette & palette){
    switch (in_bpp){
    case 8:
      return RGBtoBGR(palette[static_cast<uint8_t>(c)]);
    case 15:
    {
        //  b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 r1 r2 r3 r4 r5
        const BGRColor b = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7); // b1 b2 b3 b4 b5 b1 b2 b3
        const BGRColor g = ((c >> 2) & 0xf8) | ((c >>  7) & 0x7); // g1 g2 g3 g4 g5 g1 g2 g3
        const BGRColor r = ((c << 3) & 0xf8) | ((c >>  2) & 0x7); // r1 r2 r3 r4 r5 r1 r2 r3
        return (r << 16) | (g << 8) | b;
    }
    break;
    case 16:
    {
        //  b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 g6 r1 r2 r3 r4 r5
        const BGRColor b = ((c >> 8) & 0xf8) | ((c >> 13) & 0x7); // b1 b2 b3 b4 b5 b1 b2 b3
        const BGRColor g = ((c >> 3) & 0xfc) | ((c >>  9) & 0x3); // g1 g2 g3 g4 g5 g6 g1 g2
        const BGRColor r = ((c << 3) & 0xf8) | ((c >>  2) & 0x7); // r1 r2 r3 r4 r5 r6 r7 r8
        return (r << 16) | (g << 8) | b;
    }
    case 32:
    case 24:
      return c & 0xFFFFFF;
    default:
        LOG(LOG_ERR, "in_bpp = %d", in_bpp);
        exit(0);
    }
    return 0;
}

inline BGRColor color_encode(const BGRColor c, const uint8_t out_bpp){
    switch (out_bpp){
    case 8:
    // rrrgggbb
        return
        (((c >> 16) & 0xFF)       & 0xE0)
       |((((c >> 8) & 0xFF) >> 3) & 0x1C)
       |(((c        & 0xFF) >> 6) & 0x03);
//    // bbgggrrr
//        return
//        (((c      ) & 0xFF)       & 0xC0)
//       |((((c >> 8) & 0xFF) >> 2) & 0x38)
//       |(((c >> 16  & 0xFF) >> 5) & 0x03);
    case 15:
    // --> 0 b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 r1 r2 r3 r4 r5
        return
        // b1 b2 b3 b4 b5 b6 b7 b8 --> 0 b1 b2 b3 b4 b5 0 0 0 0 0 0 0 0 0 0
        (((c         & 0xFF) << 7) & 0x7C00)
        // g1 g2 g3 g4 g5 g6 g7 g8 --> 0 0 0 0 0 0 g1 g2 g3 g4 g5 0 0 0 0 0
       |((((c >>  8) & 0xFF) << 2) & 0x03E0)
        // r1 r2 r3 r4 r5 r6 r7 r8 --> 0 0 0 0 0 0 0 0 0 0 0 r1 r2 r3 r4 r5
       | (((c >> 16) & 0xFF) >> 3);
    case 16:
    // --> b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 g6 r1 r2 r3 r4 r5
        return
        // b1 b2 b3 b4 b5 b6 b7 b8 --> b1 b2 b3 b4 b5 0 0 0 0 0 0 0 0 0 0 0
        (((c         & 0xFF) << 8) & 0xF800)
        // g1 g2 g3 g4 g5 g6 g7 g8 --> 0 0 0 0 0 g1 g2 g3 g4 g5 g6 0 0 0 0 0
       |((((c >>  8) & 0xFF) << 3) & 0x07E0)
        // r1 r2 r3 r4 r5 r6 r7 r8 --> 0 0 0 0 0 0 0 0 0 0 0 r1 r2 r3 r4 r5
       | (((c >> 16) & 0xFF) >> 3);
    case 32:
    case 24:
        return c;
    default:
 //       assert(false);
    break;
    }
    return 0;
}


#endif

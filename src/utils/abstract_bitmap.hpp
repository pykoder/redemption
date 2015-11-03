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
   Copyright (C) Wallix 2011
   Author(s): Christophe Grosjean, Javier Caverni, Martin Potier,
              Meng Tan
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   This file implement the bitmap items data structure
   including RDP RLE compression and decompression algorithms

   It also features storage and color versionning of the bitmap
   returning a pointer on a table, corresponding to the required
   color model.
*/

#ifndef _REDEMPTION_UTILS_BITMAP_HPP__
#define _REDEMPTION_UTILS_BITMAP_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <png.h>
#include <string.h>

#include <cerrno>
#include <cassert>
#include <cstddef>
#include <utility>
#include <type_traits> // aligned_storage

//#include "error.h"
//#include "log.hpp"
//#include "bitfu.hpp"
#include "colors.hpp"
//#include "stream.hpp"
#include "ssl_calls.hpp"
#include "rect.hpp"
//#include "fdbuf.hpp"
#include "bitmap_data_allocator.hpp"

//#include "fdbuf.hpp"

using std::size_t;

static inline uint8_t nbbytes(unsigned value) noexcept
{
    return static_cast<uint8_t>((value+7) / 8);
}

static inline void out_bytes_le(uint8_t * ptr, const uint8_t nb, const unsigned value) noexcept
{
    for (uint8_t b = 0 ; b < nb ; ++b){
        ptr[b] = static_cast<uint8_t>(value >> (8 * b));
    }
}
// this name because the fonction below is only defined for 1 to 4/8 bytes (works on underlying unsigned)
static inline unsigned in_uint32_from_nb_bytes_le(const uint8_t nb, const uint8_t * ptr) noexcept
{
    unsigned res = 0;
    for (int b = 0 ; b < nb ; ++b){
        res |= ptr[b] << (8 * b);
    }
    return res;
}


class Bitmap
{
    struct DataBitmapBase {
        const uint16_t cx_;
        const uint16_t cy_;
        const uint8_t bpp_;
        uint_fast8_t counter_;

        const size_t line_size_;
        const size_t bmp_size_;
        uint8_t * const ptr_;
        // Memoize compressed bitmap
        /*mutable*/ uint8_t * data_compressed_;
        size_t size_compressed_;
        mutable uint8_t sha1_[20];
        mutable bool sha1_is_init_;

        DataBitmapBase(uint8_t bpp, uint16_t cx, uint16_t cy, uint8_t * ptr) noexcept
        : cx_((cx+3) & ~3)
        , cy_(cy)
        , bpp_(bpp)
        , counter_(1)
        , line_size_(this->cx_ * nbbytes(this->bpp_))
        , bmp_size_(this->line_size_ * cy)
        , ptr_(ptr)
        , data_compressed_(nullptr)
        , size_compressed_(0)
        , sha1_is_init_(false)
        {}

        DataBitmapBase(uint16_t cx, uint16_t cy, uint8_t * ptr) noexcept
        : cx_(cx)
        , cy_(cy)
        , bpp_(24)
        , counter_(1)
        , line_size_(this->cx_ * 3)
        , bmp_size_(this->line_size_ * cy)
        , ptr_(ptr)
        , data_compressed_(nullptr)
        , size_compressed_(0)
        , sha1_is_init_(false)
        {}
    };

    class DataBitmap : DataBitmapBase
    {
        DataBitmap(uint8_t bpp, uint16_t cx, uint16_t cy, uint8_t * ptr) noexcept
        : DataBitmapBase(bpp, cx, cy, ptr)
        {}

        DataBitmap(uint16_t cx, uint16_t cy, uint8_t * ptr) noexcept
        : DataBitmapBase(cx, cy, ptr)
        {}

        ~DataBitmap()
        {
            aux_::bitmap_data_allocator.dealloc(this->data_compressed_);
        }

        DataBitmap(DataBitmap const &);
        DataBitmap & operator=(DataBitmap const &);

        static const size_t palette_index = sizeof(typename std::aligned_storage<sizeof(DataBitmapBase), alignof(BGRColor)>::type);

    public:
        static size_t compute_bmp_size(uint8_t bpp, uint16_t cx, uint16_t cy) noexcept
        {
            return align4(cx) * nbbytes(bpp) * cy;
        }

        static DataBitmap * construct(uint8_t bpp, uint16_t cx, uint16_t cy)
        {
            const size_t sz = compute_bmp_size(bpp, cx, cy);
            const size_t sz_struct = bpp == 8 ? palette_index + sizeof(BGRPalette) : sizeof(DataBitmap);
            uint8_t * p = static_cast<uint8_t*>(aux_::bitmap_data_allocator.alloc(sz_struct + sz));
            return new (p) DataBitmap(bpp, cx, cy, p + sz_struct);
        }

        static DataBitmap * construct_png(uint16_t cx, uint16_t cy)
        {
            const size_t sz = cx * cy * 3;
            const size_t sz_struct = sizeof(DataBitmap);
            uint8_t * p = static_cast<uint8_t*>(aux_::bitmap_data_allocator.alloc(sz_struct + sz));
            return new (p) DataBitmap(cx, cy, p + sz_struct);
        }

        static void destruct(DataBitmap * cdata) {
            cdata->~DataBitmap();
            aux_::bitmap_data_allocator.dealloc(cdata);
        }

        void copy_sha1(uint8_t (&sig)[20]) const {
            if (!this->sha1_is_init_) {
                this->sha1_is_init_ = true;
                SslSha1 sha1;
                if (this->bpp_ == 8) {
                    sha1.update(this->data_palette(), sizeof(BGRPalette));
                }
                sha1.update(&this->bpp_, sizeof(this->bpp_));
                sha1.update(reinterpret_cast<const uint8_t *>(&this->cx_), sizeof(this->cx_));
                sha1.update(reinterpret_cast<const uint8_t *>(&this->cy_), sizeof(this->cy_));
                const uint8_t * first = this->get();
                const uint8_t * last = first + this->cy_ * this->line_size_;
                for (; first != last; first += this->line_size_) {
                    sha1.update(first, this->line_size_);
                }
                sha1.final(this->sha1_, 20);
            }
            memcpy(sig, this->sha1_, sizeof(this->sha1_));
        }

        uint8_t * get() const noexcept {
            return this->ptr_;
        }

    private:
        uint8_t const * data_palette() const noexcept {
            //REDASSERT(this->bpp() == 8);
            return reinterpret_cast<uint8_t const*>(this) + palette_index;
        }

    public:
        BGRPalette & palette() noexcept {
            //REDASSERT(this->bpp() == 8);
            return reinterpret_cast<BGRPalette &>(reinterpret_cast<uint8_t*>(this)[palette_index]);
        }

        const BGRPalette & palette() const noexcept {
            //REDASSERT(this->bpp() == 8);
            return reinterpret_cast<const BGRPalette &>(reinterpret_cast<const uint8_t*>(this)[palette_index]);
        }

        uint16_t cx() const noexcept {
            return this->cx_;
        }

        uint16_t cy() const noexcept {
            return this->cy_;
        }

        size_t line_size() const noexcept {
            return this->line_size_;
        }

        uint8_t bpp() const noexcept {
            return this->bpp_;
        }

        size_t bmp_size() const noexcept {
            return this->bmp_size_;
        }

        void copy_compressed_buffer(void const * data, size_t n) {
            REDASSERT(this->compressed_size() == 0);
            uint8_t * p = static_cast<uint8_t*>(aux_::bitmap_data_allocator.alloc(n));
            this->data_compressed_ = static_cast<uint8_t*>(memcpy(p, data, n));
            this->size_compressed_ = n;
        }

        const uint8_t * compressed_data() const noexcept {
            return this->data_compressed_;
        }

        size_t compressed_size() const noexcept {
            return this->size_compressed_;
        }

        void inc() noexcept {
            ++this->counter_;
        }

        void dec() noexcept {
            --this->counter_;
        }

        uint_fast8_t count() const noexcept {
            return this->counter_;
        }
    };

    DataBitmap * data_bitmap;

    void * operator new(size_t n) = delete;

public:
    Bitmap() noexcept
    : data_bitmap(nullptr)
    {}

    Bitmap(Bitmap&& bmp) noexcept
    : data_bitmap(bmp.data_bitmap)
    {
        bmp.data_bitmap = nullptr;
    }

    Bitmap(const Bitmap & other)
    : data_bitmap(other.data_bitmap)
    {
        if (this->data_bitmap) {
            this->data_bitmap->inc();
        }
    }

    ~Bitmap() {
        this->reset();
    }

    Bitmap & operator=(const Bitmap & other)
    {
        other.data_bitmap->inc();
        this->reset();
        this->data_bitmap = other.data_bitmap;
        return *this;
    }

    Bitmap & operator=(Bitmap && other) noexcept
    {
        auto other_data = other.data_bitmap;
        other.data_bitmap = nullptr;
        this->data_bitmap = other_data;
        return *this;
    }

    bool is_valid() const noexcept {
        return this->data_bitmap;
    }

    void reset() {
        if (this->data_bitmap) {
            this->data_bitmap->dec();
            if (this->data_bitmap->count() == 0) {
                DataBitmap::destruct(this->data_bitmap);
            }
            this->data_bitmap = nullptr;
        }
    }

    void swap(Bitmap & other) noexcept {
        using std::swap;
        swap(this->data_bitmap, other.data_bitmap);
    }

    //Bitmap(uint8_t bpp, uint16_t cx, uint16_t cy, const BGRPalette * palette);

    Bitmap(uint8_t session_color_depth, uint8_t bpp, const BGRPalette * palette,
           uint16_t cx, uint16_t cy, const uint8_t * data, const size_t size,
           bool compressed = false)
    : data_bitmap(DataBitmap::construct(bpp, cx, cy))
    {
        if (cx <= 0 || cy <= 0){
            LOG(LOG_ERR, "Bogus empty bitmap!!! cx=%u cy=%u size=%u bpp=%u", cx, cy, size, bpp);
        }

        if (bpp == 8){
            if (palette){
                this->data_bitmap->palette() = *palette;
            }
            else {
                this->data_bitmap->palette() = BGRPalette::classic_332();
            }
        }

        //LOG(LOG_INFO, "Creating bitmap (%p) cx=%u cy=%u size=%u bpp=%u", this, cx, cy, size, bpp);

        if (compressed) {
            this->data_bitmap->copy_compressed_buffer(data, size);

            if ((session_color_depth == 32) && ((bpp == 24) || (bpp == 32))) {
                this->decompress60(cx, cy, data, size);
            }
            else {
                this->decompress(data, cx, cy, size);
            }
        } else {
            uint8_t * dest = this->data_bitmap->get();
            const uint8_t * src = data;
            const size_t data_width = cx * nbbytes(bpp);
            const size_t line_size = this->line_size();
            const uint16_t cy = this->cy();
            for (uint16_t i = 0; i < cy ; i++){
                memcpy(dest, src, data_width);
                memset(dest + line_size, 0, line_size - data_width);
                src += data_width;
                dest += line_size;
            }
        }
    }

    Bitmap(const Bitmap & src_bmp, const Rect & r)
    : data_bitmap(src_bmp.data_bitmap)
    {
        //LOG(LOG_INFO, "Creating bitmap (%p) extracting part cx=%u cy=%u size=%u bpp=%u", this, cx, cy, bmp_size, bpp);

        if (0 == r.x && 0 == r.y && r.cx == src_bmp.cx() && r.cy == src_bmp.cy()) {
            this->data_bitmap->inc();
            return ;
        }

        this->data_bitmap = DataBitmap::construct(src_bmp.bpp(), r.cx, r.cy);
        if (this->bpp() == 8){
            this->data_bitmap->palette() = src_bmp.data_bitmap->palette();
        }

        // bitmapDataStream (variable): A variable-sized array of bytes.
        //  Uncompressed bitmap data represents a bitmap as a bottom-up,
        //  left-to-right series of pixels. Each pixel is a whole
        //  number of bytes. Each row contains a multiple of four bytes
        // (including up to three bytes of padding, as necessary).

        // In redemption we ensure a more constraint restriction to avoid padding
        // bitmap width must always be a multiple of 4

        const uint8_t Bpp = nbbytes(this->bpp());
        uint8_t *dest = this->data_bitmap->get();
        const size_t line_size = this->line_size();
        const size_t src_line_size = src_bmp.line_size();
        const uint16_t cy = this->cy();
        const uint16_t src_cy = src_bmp.cy();
        const uint8_t *src = src_bmp.data_bitmap->get() + src_line_size * (src_cy - r.y - cy) + r.x * Bpp;
        const unsigned line_to_copy = r.cx * Bpp;

        for (uint16_t i = 0; i < cy; i++) {
            memcpy(dest, src, line_to_copy);
            if (line_to_copy < line_size){
                memset(dest + line_to_copy, 0, line_size - line_to_copy);
            }
            src += src_line_size;
            dest += line_size;
        }
    }

    TODO("add palette support")
    Bitmap(const uint8_t * vnc_raw, uint16_t vnc_cx, uint16_t /*vnc_cy*/, uint8_t vnc_bpp, const Rect & tile)
    : data_bitmap(DataBitmap::construct(vnc_bpp, tile.cx, tile.cy))
    {
        //LOG(LOG_INFO, "Creating bitmap (%p) extracting part cx=%u cy=%u size=%u bpp=%u", this, cx, cy, bmp_size, bpp);

        // raw: vnc data is a bunch of pixels of size cx * cy * nbbytes(bpp)
        // line 0 is the first line (top-up)

        // bitmapDataStream (variable): A variable-sized array of bytes.
        //  Uncompressed bitmap data represents a bitmap as a bottom-up,
        //  left-to-right series of pixels. Each pixel is a whole
        //  number of bytes. Each row contains a multiple of four bytes
        // (including up to three bytes of padding, as necessary).

        const uint8_t Bpp = nbbytes(this->bpp());
        const unsigned src_row_size = vnc_cx * Bpp;
        uint8_t *dest = this->data_bitmap->get();
        const uint8_t *src = vnc_raw + src_row_size * (tile.y + tile.cy - 1) + tile.x * Bpp;
        const uint16_t line_to_copy_size = tile.cx * Bpp;
        const size_t line_size = this->line_size();
        const uint16_t cy = this->cy();

        for (uint16_t i = 0; i < cy; i++) {
            memcpy(dest, src, line_to_copy_size);
            if (line_to_copy_size < line_size){
                memset(dest + line_to_copy_size, 0, line_size - line_to_copy_size);
            }
            src -= src_row_size;
            dest += line_size;
        }
    }

    const uint8_t* data() const noexcept {
        return this->data_bitmap->get();
    }

    const BGRPalette & palette() const noexcept {
        return this->data_bitmap->palette();
    }

    uint16_t cx() const noexcept {
        return this->data_bitmap->cx();
    }

    uint16_t cy() const noexcept {
        return this->data_bitmap->cy();
    }

    size_t line_size() const noexcept {
        return this->data_bitmap->line_size();
    }

    uint8_t bpp() const noexcept {
        return this->data_bitmap->bpp();
    }

    size_t bmp_size() const noexcept {
        return this->data_bitmap->bmp_size();
    }

    bool open_png_file(const char * filename) {
        this->reset();

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                     nullptr, nullptr, nullptr);
        if (!png_ptr) {
            return false;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);
            return false;
        }
        // this handle lib png errors for this call
        if (setjmp(png_ptr->jmpbuf)) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            return false;
        }

        FILE * fd = fopen(filename, "rb");
        if (!fd) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            return false;
        }
        png_init_io(png_ptr, fd);

        png_read_info(png_ptr, info_ptr);

        png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_byte color_type = png_get_color_type(png_ptr, info_ptr);

        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_gray_1_2_4_to_8(png_ptr);

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr);

        if (bit_depth == 16)
            png_set_strip_16(png_ptr);
        else if (bit_depth < 8)
            png_set_packing(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        if (color_type & PNG_COLOR_MASK_ALPHA) {
            png_set_strip_alpha(png_ptr);
        }
        png_set_bgr(png_ptr);
        png_read_update_info(png_ptr, info_ptr);

        TODO("Looks like there's a shift when width is not divisible by 4");
        png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        if (static_cast<uint16_t>(width) * 3 != rowbytes) {
            LOG(LOG_ERR, "PNG Image has bad type");
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            return false;
        }

        this->data_bitmap = DataBitmap::construct_png(static_cast<uint16_t>(width), static_cast<uint16_t>(height));
        png_bytep row = this->data_bitmap->get() + rowbytes * height - rowbytes;
        png_bytep * row_pointers = new png_bytep[height];
        for (uint i = 0; i < height; ++i) {
            row_pointers[i] = row - i * rowbytes;
        }
        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, info_ptr);
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        fclose(fd);
        delete [] row_pointers;

        // hexdump_d(this->data_bitmap->get(), this->bmp_size);

        return true;
    } // bool open_png_file(const char * filename)


private:
    //TODO("move that function to external definition")
    //const char * get_opcode(uint8_t opcode){
    //    enum {
    //        FILL    = 0,
    //        MIX     = 1,
    //        FOM     = 2,
    //        COLOR   = 3,
    //        COPY    = 4,
    //        MIX_SET = 6,
    //        FOM_SET = 7,
    //        BICOLOR = 8,
    //        SPECIAL_FGBG_1 = 9,
    //        SPECIAL_FGBG_2 = 10,
    //        WHITE = 13,
    //        BLACK = 14
    //    };
    //
    //    switch (opcode){
    //        case FILL:
    //            return "FILL";
    //        case MIX:
    //            return "MIX";
    //        case FOM:
    //            return "FOM";
    //        case COLOR:
    //            return "COLOR";
    //        case COPY:
    //            return "COPY";
    //        case MIX_SET:
    //            return "MIX_SET";
    //        case FOM_SET:
    //            return "FOM_SET";
    //        case BICOLOR:
    //            return "BICOLOR";
    //        case SPECIAL_FGBG_1:
    //            return "SPECIAL_FGBG_1";
    //        case SPECIAL_FGBG_2:
    //            return "SPECIAL_FGBG_2";
    //        case WHITE:
    //            return "WHITE";
    //        case BLACK:
    //            return "BLACK";
    //        default:
    //            return "Unknown Opcode";
    //    };
    //}

    void decompress(const uint8_t* input, uint16_t src_cx, uint16_t src_cy, size_t size) const
    {
        const uint8_t Bpp = nbbytes(this->bpp());
        const uint16_t dst_cx = this->cx();
        uint8_t* pmin = this->data_bitmap->get();
        uint8_t* pmax = pmin + this->bmp_size();
        const size_t line_size = this->line_size();
        uint16_t out_x_count = 0;
        unsigned yprev = 0;
        uint8_t* out = pmin;
        const uint8_t* end = input + size;
        unsigned color1 = 0;
        unsigned color2 = 0;
        unsigned mix = 0xFFFFFFFF;
        unsigned mask = 0;
        unsigned fom_mask = 0;
        unsigned count = 0;
        int bicolor = 0;

        enum {
            FILL    = 0,
            MIX     = 1,
            FOM     = 2,
            COLOR   = 3,
            COPY    = 4,
            MIX_SET = 6,
            FOM_SET = 7,
            BICOLOR = 8,
            SPECIAL_FGBG_1 = 9,
            SPECIAL_FGBG_2 = 10,
            WHITE = 13,
            BLACK = 14
        };

        uint8_t opcode;
        uint8_t lastopcode = 0xFF;

        while (input < end) {

            // Read RLE operators, handle short and long forms
            uint8_t code = input[0]; input++;

            switch (code >> 4) {
            case 0xf:
                switch (code){
                    case 0xFD:
                        opcode = WHITE;
                        count = 1;
                    break;
                    case 0xFE:
                        opcode = BLACK;
                        count = 1;
                    break;
                    case 0xFA:
                        opcode = SPECIAL_FGBG_2;
                        count = 8;
                    break;
                    case 0xF9:
                        opcode = SPECIAL_FGBG_1;
                        count = 8;
                    break;
                    case 0xF8:
                        opcode = code & 0xf;
                        assert(opcode != 11 && opcode != 12 && opcode != 15);
                        count = input[0]|(input[1] << 8);
                        count += count;
                        input += 2;
                    break;
                    default:
                        opcode = code & 0xf;
                        assert(opcode != 11 && opcode != 12 && opcode != 15);
                        count = input[0]|(input[1] << 8);
                        input += 2;
                        // Opcodes 0xFB, 0xFC, 0xFF are some unknown orders of length 1 ?
                    break;
                }
            break;
            case 0x0e: // Bicolor, short form (1 or 2 bytes)
                opcode = BICOLOR;
                count = code & 0xf;
                if (!count){
                    count = input[0] + 16; input++;
                }
                count += count;
                break;
            case 0x0d:  // FOM SET, short form  (1 or 2 bytes)
                opcode = FOM_SET;
                count = code & 0x0F;
                if (count){
                    count <<= 3;
                }
                else {
                    count = input[0] + 1; input++;
                }
            break;
            case 0x05:
            case 0x04:  // FOM, short form  (1 or 2 bytes)
                opcode = FOM;
                count = code & 0x1F;
                if (count){
                    count <<= 3;
                }
                else {
                    count = input[0] + 1; input++;
                }
            break;
            case 0x0c: // MIX SET, short form (1 or 2 bytes)
                opcode = MIX_SET;
                count = code & 0x0f;
                if (!count){
                    count = input[0] + 16; input++;
                }
            break;
            default:
                opcode = static_cast<uint8_t>(code >> 5); // FILL, MIX, FOM, COLOR, COPY
                count = code & 0x1f;
                if (!count){
                    count = input[0] + 32; input++;
                }

                assert(opcode < 5);
                break;
            }

            /* Read preliminary data */
            switch (opcode) {
            case FOM:
                mask = 1;
                fom_mask = input[0]; input++;
            break;
            case SPECIAL_FGBG_1:
                mask = 1;
                fom_mask = 3;
            break;
            case SPECIAL_FGBG_2:
                mask = 1;
                fom_mask = 5;
            break;
            case BICOLOR:
                bicolor = 0;
                color1 = this->get_pixel(Bpp, input);
                input += Bpp;
                color2 = this->get_pixel(Bpp, input);
                input += Bpp;
                break;
            case COLOR:
                color2 = this->get_pixel(Bpp, input);
                input += Bpp;
                break;
            case MIX_SET:
                mix = this->get_pixel(Bpp, input);
                input += Bpp;
            break;
            case FOM_SET:
                mix = this->get_pixel(Bpp, input);
                input += Bpp;
                mask = 1;
                fom_mask = input[0]; input++;
                break;
            default: // for FILL, MIX or COPY nothing to do here
                break;
            }

            // MAGIC MIX of one pixel to comply with crap in Bitmap RLE compression
            if ((opcode == FILL)
            && (opcode == lastopcode)
            && (out != pmin + line_size)){
                yprev = (out - line_size < pmin) ? 0 : this->get_pixel(Bpp, out - line_size);
                out_bytes_le(out, Bpp, yprev ^ mix);
                count--;
                out += Bpp;
                out_x_count += 1;
                if (out_x_count == dst_cx){
                    memset(out, 0, (dst_cx - src_cx) * Bpp);
                    out_x_count = 0;
                }
            }
            lastopcode = opcode;

            //LOG(LOG_INFO, "%s %u", this->get_opcode(opcode), count);

            /* Output body */
            while (count > 0) {
                if(out >= pmax) {
                    LOG(LOG_WARNING, "Decompressed bitmap too large. Dying.");
                    throw Error(ERR_BITMAP_DECOMPRESSED_DATA_TOO_LARGE);
                }
                yprev = (out - line_size < pmin) ? 0 : this->get_pixel(Bpp, out - line_size);

                switch (opcode) {
                case FILL:
                    out_bytes_le(out, Bpp, yprev);
                    break;
                case MIX_SET:
                case MIX:
                    out_bytes_le(out, Bpp, yprev ^ mix);
                    break;
                case FOM_SET:
                case FOM:
                    if (mask == 0x100){
                        mask = 1;
                        fom_mask = input[0]; input++;
                    }
                case SPECIAL_FGBG_1:
                case SPECIAL_FGBG_2:
                    if (mask & fom_mask){
                        out_bytes_le(out, Bpp, yprev ^ mix);
                    }
                    else {
                        out_bytes_le(out, Bpp, yprev);
                    }
                    mask <<= 1;
                    break;
                case COLOR:
                    out_bytes_le(out, Bpp, color2);
                    break;
                case COPY:
                    out_bytes_le(out, Bpp, this->get_pixel(Bpp, input));
                    input += Bpp;
                    break;
                case BICOLOR:
                    if (bicolor) {
                        out_bytes_le(out, Bpp, color2);
                        bicolor = 0;
                    }
                    else {
                        out_bytes_le(out, Bpp, color1);
                        bicolor = 1;
                    }
                break;
                case WHITE:
                    out_bytes_le(out, Bpp, 0xFFFFFFFF);
                break;
                case BLACK:
                    out_bytes_le(out, Bpp, 0);
                break;
                default:
                    assert(false);
                    break;
                }
                count--;
                out += Bpp;
                out_x_count += 1;
                if (out_x_count == dst_cx){
                    memset(out, 0, (dst_cx - src_cx) * Bpp);
                    out_x_count = 0;
                }
            }
        }
        return;
    }



// [MS-RDPEGDI] 2.2.2.5.1 RDP 6.0 Bitmap Compressed Bitmap Stream
//  (RDP6_BITMAP_STREAM)
// ==============================================================

// The RDP6_BITMAP_STREAM structure contains a stream of bitmap data
//  compressed using RDP 6.0 Bitmap Compression techniques (section 3.1.9).
//  Depending on the compression techniques employed, the bitmap data is
//  represented using the AYCoCg or ARGB color space (section 3.1.9.1.2).

// Compressed bitmap data is sent encapsulated in a Bitmap Update
//  ([MS-RDPBCGR] section 2.2.9.1.1.3.1.2), Fast-Path Bitmap Update
//  ([MS-RDPBCGR] section 2.2.9.1.2.1.2 ), Cache Bitmap -Revision 1 (section
//  2.2.2.2.1.2.2) Secondary Drawing Orders, or Cache Bitmap - Revision 2
//  (section 2.2.2.2.1.2.3) Secondary Drawing Orders. In all of these cases,
//  the data is encapsulated inside a Bitmap Data structure ([MS-RDPBCGR]
//  section 2.2.9.1.1.3.1.2.2).

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  FormatHeader |             AlphaPlane (variable)             |
// +---------------+-----------------------------------------------+
// |                              ...                              |
// +---------------------------------------------------------------+
// |                   LumaOrRedPlane (variable)                   |
// +---------------------------------------------------------------+
// |                              ...                              |
// +---------------------------------------------------------------+
// |              OrangeChromaOrGreenPlane (variable)              |
// +---------------------------------------------------------------+
// |                              ...                              |
// +---------------------------------------------------------------+
// |               GreenChromaOrBluePlane (variable)               |
// +---------------------------------------------------------------+
// |                              ...                              |
// +---------------+-----------------------------------------------+
// | Pad (optional)|
// +---------------+

// FormatHeader (1 byte): An 8-bit, unsigned integer. This field contains a
//  1-byte, bit-packed update header as follows.

//  The format of the update header byte is described by the following
//   bitmask diagram.

//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
//  |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  | CLL |C|R|N|Res|
//  |     |S|L|A|erv|
//  |     | |E| | ed|
//  +-----+-+-+-+---+

//   CLL (3 bits): A 3-bit, unsigned integer field that indicates the Color
//    Loss Level (section 3.1.9.1.4). If CLL is set to 0, the color space
//    used is ARGB. Otherwise, CLL MUST be in the range 1 to 7 (inclusive),
//    and the color space used is AYCoCg.

//   CS (1 bit): A 1-bit field that indicates whether chroma subsampling is
//    being used (section 3.1.9.1.3). If CS is equal to 1, chroma subsampling
//    is being used, and the CLL field MUST be greater than 0, as chroma
//    subsampling applies only to the AYCoCg color space.

//   RLE (1 bit): A 1-bit field. If RLE is equal to 1, RDP 6.0 RLE is used to
//    compress the color planes (section 3.1.9.2). If not, RLE is equal to 0,
//    and the color plane is sent uncompressed.

//   NA (1 bit): A 1-bit field. Indicates if an alpha plane is present. If NA
//    is equal to 1, there is no alpha plane. The values of the alpha plane
//    are then assumed to be 0xFF (fully opaque), and the bitmap data
//    contains only three color planes. If NA is equal to 0, the alpha plane
//    is sent as the first color plane.

//   Reserved (2 bits): A 2-bit, unsigned integer field. Reserved for future
//    use.

// AlphaPlane (variable): A variable-length field that contains the alpha
//  plane. If the RLE subfield in the FormatHeader indicates that all of the
//  color planes are RLE compressed (section 3.1.9.2), this field contains an
//  RDP 6.0 RLE Segments (section 2.2.2.5.1.1) structure. Otherwise, it
//  contains the raw bytes of the color plane.

// LumaOrRedPlane (variable): A variable-length field that contains the luma
//  plane (AYCoCg color space) or the red plane (ARGB color space). If the
//  CLL subfield of the FormatHeader is greater than 0, the AYCoCg color
//  space MUST be used. Otherwise, the ARGB color space MUST be used.

//  If the RLE subfield in the FormatHeader indicates that all of the color
//   planes are RLE compressed (section 3.1.9.2), this field contains an RDP
//   6.0 RLE Segments (section 2.2.2.5.1.1) structure. Otherwise, it contains
//   the raw bytes of the color plane.

// OrangeChromaOrGreenPlane (variable): A variable-length field that contains
//  the orange chroma plane (AYCoCg color space) or the green plane (ARGB
//  color space). If the CLL subfield of the FormatHeader is greater than 0,
//  the AYCoCg color space MUST be used. Otherwise, the ARGB color space MUST
//  be used.

//  If the RLE subfield in the FormatHeader indicates that all of the color
//   planes are RLE compressed (section 3.1.9.2), this field contains an RDP
//   6.0 RLE Segments (section 2.2.2.5.1.1) structure. Otherwise, it contains
//   the raw bytes of the color plane.

//  Depending on the values of the CLL and CS subfields of the FormatHeader
//   (in the case of the AYCoCg color space), the orange chroma plane may
//   have been transformed by color loss reduction (section 3.1.9.1.4) and
//   chroma subsampling (section 3.1.9.1.3).

// GreenChromaOrBluePlane (variable): A variable-length field that contains
//  the green chroma plane (AYCoCg color space) or the blue plane (ARGB color
//  space). If the CLL subfield of the FormatHeader is greater than 0, the
//  AYCoCg color space MUST be used. Otherwise, the ARGB color space MUST be
//  used.

//  If the RLE subfield in the FormatHeader indicates that all of the color
//   planes are RLE compressed (section 3.1.9.2), this field contains an RDP
//   6.0 RLE Segments (section 2.2.2.5.1.1) structure. Otherwise, it contains
//   the raw bytes of the color plane.

//  Depending on the values of the CLL and CS subfields of the FormatHeader
//   (in the case of the AYCoCg color space), the green chroma plane may have
//   been transformed by color loss reduction (section 3.1.9.1.4) and chroma
//   subsampling (section 3.1.9.1.3).

// Pad (1 byte): An 8-bit, unsigned integer containing padding values that
//  MUST be ignored. This optional field is only present if the RLE subfield
//  of the FormatHeader field is zero.

public:
    static inline void decompress_color_plane(uint16_t src_cx, uint16_t src_cy, const uint8_t *& data,
         size_t & data_size, uint16_t cx, uint8_t * color_plane)
    {
        uint32_t   size        = sizeof(uint8_t) * src_cx * src_cy;
        uint16_t   line_size   = src_cx;
        uint8_t  * line_start  = color_plane;
        uint8_t  * write_point = line_start;

        while (size) {
            //LOG(LOG_INFO, "size=%u data_size=%u", size, data_size);
            REDASSERT(data_size);

            uint8_t controlByte = *data++;
            data_size--;

            uint8_t nRunLength =  (controlByte & 0x0F);
            uint8_t cRawBytes  = ((controlByte & 0xF0) >> 4);

            //LOG(LOG_INFO, "    nRunLength=%d cRawBytes=%d", nRunLength, cRawBytes);

            if (nRunLength == 1) {
                nRunLength = 16 + cRawBytes;
                cRawBytes  = 0;
            }
            else if (nRunLength == 2) {
                if (cRawBytes == 15) {
                    nRunLength = 47;
                    cRawBytes  = 0;
                }
                else {
                    nRunLength = 32 + cRawBytes;
                    cRawBytes  = 0;
                }
            }

            //LOG(LOG_INFO, "(1) nRunLength=%d cRawBytes=%d", nRunLength, cRawBytes);

            if (cRawBytes) {
                ::memcpy(write_point, data, cRawBytes);
                write_point += cRawBytes;
                data        += cRawBytes;
                data_size   -= cRawBytes;
                line_size   -= cRawBytes;
                size        -= cRawBytes;
            }

            if (nRunLength) {
                ::memset(write_point, ((line_size == src_cx) ? 0 : *(write_point - 1)), nRunLength);
                write_point += nRunLength;
                line_size   -= nRunLength;
                size        -= nRunLength;
            }

            if (!line_size) {
                line_size  =  src_cx;
                line_start += cx;
            }
        }

        for (uint8_t * ypos_begin = color_plane + cx, * ypos_end = color_plane + cx * src_cy;
             ypos_begin < ypos_end; ypos_begin += cx) {
            for (uint8_t * xpos_begin = ypos_begin, * xpos_end = xpos_begin + src_cx; xpos_begin < xpos_end; xpos_begin++) {
                uint8_t delta = *xpos_begin;

                //LOG(LOG_INFO, "delta=%d", delta);

                if ((delta % 2) == 0) {
                    delta >>= 1;
                    //LOG(LOG_INFO, "delta(o)=%02x", delta);
                }
                else {
                    delta = 255 - ((delta - 1) >> 1);
                    //LOG(LOG_INFO, "delta(e)=%02x", delta);
                }

                *xpos_begin = static_cast<uint8_t>(*(xpos_begin - cx) + delta);

                //LOG(LOG_INFO, "delta(1)=%d", (*xpos_begin));
            }
        }
    }

private:
    static inline void in_copy_color_plan(uint16_t src_cx, uint16_t src_cy, const uint8_t * & data,
         size_t & data_size, uint16_t cx, uint8_t * color_plane)
    {
        uint8_t * line_start  = color_plane;

        for (uint16_t y = 0; y < src_cy; y++) {
            ::memcpy(line_start, data, src_cx);

            data       += src_cx;
            data_size  -= src_cx;
            line_start += cx;
        }
    }

    void decompress60(uint16_t src_cx, uint16_t src_cy, const uint8_t * data, size_t data_size) const
    {
        //LOG(LOG_INFO, "bmp decompress60: cx=%u cy=%u data_size=%u", src_cx, src_cy, data_size);

        REDASSERT((this->bpp() == 24) || (this->bpp() == 32));

        //LOG(LOG_INFO, "data_size=%u src_cx=%u src_cy=%u", data_size, src_cx, src_cy);
        //hexdump_d(data, data_size);
        uint8_t FormatHeader = *data++;
        data_size--;

        uint8_t color_loss_level   =   (FormatHeader & 0x07);
        bool    chroma_subsampling = (((FormatHeader & 0x08) >> 3) == 1);
        bool    rle                = (((FormatHeader & 0x10) >> 4) == 1);
        bool    no_alpha_plane     = (((FormatHeader & 0x20) >> 5) == 1);

        //LOG(LOG_INFO, "FormatHeader=0x%02X color_loss_level=%d chroma_subsampling=%s rle=%s no_alpha_plane=%s",
        //    FormatHeader, color_loss_level, (chroma_subsampling ? "yes" : "no"), (rle ? "yes" : "no"),
        //    (no_alpha_plane ? "yes" : "no"));

        if (color_loss_level || chroma_subsampling) {
            LOG(LOG_INFO, "Unsupported compression options", color_loss_level & (chroma_subsampling << 3));
            return;
        }

        const uint16_t cx = this->cx();
        const uint16_t cy = this->cy();

        const uint32_t color_plane_size = sizeof(uint8_t) * cx * cy;

        uint8_t * mem_color   = static_cast<uint8_t *>(alloca(color_plane_size * 3));
        uint8_t * red_plane   = mem_color + color_plane_size * 0;
        uint8_t * green_plane = mem_color + color_plane_size * 1;
        uint8_t * blue_plane  = mem_color + color_plane_size * 2;

        if (rle) {
            if (!no_alpha_plane) {
                this->decompress_color_plane(src_cx, src_cy, data, data_size, cx, red_plane);
            }

            this->decompress_color_plane(src_cx, src_cy, data, data_size, cx, red_plane);
            this->decompress_color_plane(src_cx, src_cy, data, data_size, cx, green_plane);
            this->decompress_color_plane(src_cx, src_cy, data, data_size, cx, blue_plane);
        }
        else {
            if (!no_alpha_plane) {
                const uint32_t size = sizeof(uint8_t) * src_cx * src_cy;
                data      += size;
                data_size -= size;
            }

            this->in_copy_color_plan(src_cx, src_cy, data, data_size, cx, red_plane);
            this->in_copy_color_plan(src_cx, src_cy, data, data_size, cx, green_plane);
            this->in_copy_color_plan(src_cx, src_cy, data, data_size, cx, blue_plane);

            data_size--;    // Pad
        }


        //LOG(LOG_INFO, "data_size=%u", data_size);
        REDASSERT(!data_size);

        uint8_t * r     = red_plane;
        uint8_t * g     = green_plane;
        uint8_t * b     = blue_plane;
        uint8_t * pixel = this->data_bitmap->get();
        uint8_t   bpp   = nbbytes(this->bpp());

        for (uint16_t y = 0; y < cy; y++) {
            for (uint16_t x = 0; x < cx; x++) {
                uint32_t color = (0xFF << 24) | ((*r++) << 16) | ((*g++) << 8) | (*b++);

                ::out_bytes_le(pixel, bpp, color);
                pixel += bpp;
            }
        }

        //LOG(LOG_INFO, "bmp decompress60: done");
    }

public:
    enum {
        FLAG_NONE = 0,
        FLAG_FILL = 1,
        FLAG_MIX  = 2,
        FLAG_FOM  = 3,
        FLAG_MIX_SET = 6,
        FLAG_FOM_SET = 7,
        FLAG_COLOR = 8,
        FLAG_BICOLOR = 9
    };

    unsigned get_pixel(const uint8_t Bpp, const uint8_t * const p) const
    {
        return in_uint32_from_nb_bytes_le(Bpp, p);
    }

    unsigned get_pixel_above(const uint8_t Bpp, const uint8_t * pmin, const uint8_t * const p) const
    {
        return ((p-this->line_size()) < pmin)
        ? 0
        : this->get_pixel(Bpp, p - this->line_size());
    }

    unsigned get_color_count(const uint8_t Bpp, const uint8_t * pmax, const uint8_t * p, unsigned color) const
    {
        unsigned acc = 0;
        while (p < pmax && this->get_pixel(Bpp, p) == color){
            acc++;
            p = p + Bpp;
        }
        return acc;
    }

    unsigned get_bicolor_count(const uint8_t Bpp, const uint8_t * pmax, const uint8_t * p, unsigned color1, unsigned color2) const
    {
        unsigned acc = 0;
        while ((p < pmax)
            && (color1 == this->get_pixel(Bpp, p))
            && (p + Bpp < pmax)
            && (color2 == this->get_pixel(Bpp, p + Bpp))) {
                acc = acc + 2;
                p = p + 2 * Bpp;
        }
        return acc;
    }

    unsigned get_fill_count(const uint8_t Bpp, const uint8_t * pmin, const uint8_t * pmax, const uint8_t * p) const
    {
        unsigned acc = 0;
        while  (p + Bpp <= pmax) {
            unsigned pixel = this->get_pixel(Bpp, p);
            unsigned ypixel = this->get_pixel_above(Bpp, pmin, p);
            if (ypixel != pixel){
                break;
            }
            p += Bpp;
            acc += 1;
        }
        return acc;
    }

    unsigned get_mix_count(const uint8_t Bpp, const uint8_t * pmin, const uint8_t * pmax, const uint8_t * p, unsigned foreground) const
    {
        unsigned acc = 0;
        while (p + Bpp <= pmax){
            if (this->get_pixel_above(Bpp, pmin, p) ^ foreground ^ this->get_pixel(Bpp, p)){
                break;
            }
            p += Bpp;
            acc += 1;
        }
        return acc;
    }

    unsigned get_fom_count(const uint8_t Bpp, const uint8_t * pmin, const uint8_t * pmax, const uint8_t * p, unsigned foreground, bool fill) const
    {
        unsigned acc = 0;
        while (true){
            unsigned count = 0;
            while  (p + Bpp <= pmax) {
                unsigned pixel = this->get_pixel(Bpp, p);
                unsigned ypixel = this->get_pixel_above(Bpp, pmin, p);
                if (ypixel ^ pixel ^ (fill?0:foreground)){
                    break;
                }
                p += Bpp;
                count += 1;
                if (count >= 9) {
                    return acc;
                }
            }
            if (!count){
                break;
            }
            acc += count;
            fill ^= true;
        }
        return acc;
    }

    void get_fom_masks(const uint8_t Bpp, const uint8_t * pmin, const uint8_t * p, uint8_t * mask, const unsigned count) const
    {
        unsigned i = 0;
        for (i = 0; i < count; i += 8)
        {
            mask[i>>3] = 0;
        }
        for (i = 0 ; i < count; i++, p += Bpp)
        {
            if (get_pixel(Bpp, p) != get_pixel_above(Bpp, pmin, p)){
                mask[i>>3] |= static_cast<uint8_t>(0x01 << (i & 7));
            }
        }
    }

    unsigned get_fom_count_set(const uint8_t Bpp, const uint8_t * pmin, const uint8_t * pmax, const uint8_t * p, unsigned & foreground, unsigned & flags) const
    {
        // flags : 1 = fill, 2 = MIX, 3 = (1+2) = FOM
        flags = FLAG_FILL;
        unsigned fill_count = this->get_fill_count(Bpp, pmin, pmax, p);
        if (fill_count) {
            if (fill_count < 8) {
                unsigned fom_count = this->get_fom_count(Bpp, pmin, pmax, p + fill_count * Bpp, foreground, false);
                if (fom_count){
                    flags = FLAG_FOM;
                    fill_count += fom_count;
                }
            }
            return fill_count;
        }
        // fill_count and mix_count can't match at the same time.
        // this would mean that foreground is black, and we will never set
        // it to black, as it's useless because fill_count allready does that.
        // Hence it's ok to check them independently.
        if  (p + Bpp <= pmax) {
            flags = FLAG_MIX;
            // if there is a pixel we are always able to mix (at worse we will set foreground ourself)
            foreground = this->get_pixel_above(Bpp, pmin, p) ^ this->get_pixel(Bpp, p);
            unsigned mix_count = 1 + this->get_mix_count(Bpp, pmin, pmax, p + Bpp, foreground);
            if (mix_count < 8) {
                unsigned fom_count = 0;
                fom_count = this->get_fom_count(Bpp, pmin, pmax, p + mix_count * Bpp, foreground, true);
                if (fom_count){
                    flags = FLAG_FOM;
                    mix_count += fom_count;
                }
            }
            return mix_count;
        }
        flags = FLAG_NONE;
        return 0;
    }


    static void get_run(const uint8_t * data, uint16_t data_size, uint8_t last_raw, uint32_t & run_length,
        uint32_t & raw_bytes)
    {
        const uint8_t * data_save = data;

        run_length = 0;
        raw_bytes  = 0;

        while (data_size) {
            raw_bytes++;
            data_size--;

            //LOG(LOG_INFO, "row_value=%c", *data);
            uint8_t last_raw_value = *(data++);

            for (; data_size && (*data == last_raw_value); run_length++, data_size--, data++)
                /*LOG(LOG_INFO, "run=%c", *data)*/;

            if (run_length >= 3) {
                break;
            }

            raw_bytes += run_length;

            run_length = 0;
        }

        if ((raw_bytes == 1) && run_length && (*data_save == last_raw)) {
            // [MS-RDPEGDI] Previous base value assumed to be 0.
            raw_bytes = 0;
            run_length++;
        }

        //LOG(LOG_INFO, "");
    }

public:
    void compute_sha1(uint8_t (&sig)[20]) const
    {
        this->data_bitmap->copy_sha1(sig);
    }

    static size_t compute_bmp_size(uint8_t bpp, uint16_t cx, uint16_t cy)
    {
        return DataBitmap::compute_bmp_size(bpp, cx, cy);
    }

    Bitmap(uint8_t out_bpp, const Bitmap& bmp)
    {
        //LOG(LOG_INFO, "Creating bitmap (%p) (copy constructor) cx=%u cy=%u size=%u bpp=%u", this, cx, cy, bmp_size, bpp);

        if (out_bpp != bmp.bpp()) {
            this->data_bitmap = DataBitmap::construct(out_bpp, bmp.cx(), bmp.cy());

            uint8_t * dest = this->data_bitmap->get();
            const uint8_t * src = bmp.data_bitmap->get();
            const uint8_t src_nbbytes = nbbytes(bmp.bpp());
            const uint8_t Bpp = nbbytes(out_bpp);

            for (size_t y = 0; y < bmp.cy() ; y++) {
                for (size_t x = 0; x < bmp.cx() ; x++) {
                    uint32_t pixel = in_uint32_from_nb_bytes_le(src_nbbytes, src);

                    pixel = color_decode(pixel, bmp.bpp(), bmp.palette());
                    if (out_bpp == 16 || out_bpp == 15 || out_bpp == 8){
                        pixel = RGBtoBGR(pixel);
                    }
                    pixel = color_encode(pixel, out_bpp);

                    out_bytes_le(dest, Bpp, pixel);
                    src += src_nbbytes;
                    dest += Bpp;
                }
                //TODO("padding code should not be necessary as source bmp width is already aligned");
                //if (this->line_size < bmp.cx() * Bpp){
                //    uint16_t padding = this->line_size - bmp.cx() * Bpp;
                //    memset(dest, 0, padding);
                //    dest += padding;
                //}
                //TODO("padding code should not be necessary for source either as source bmp width is already aligned");
                //src += bmp.line_size - bmp.cx * src_nbbytes;
            }

            if (out_bpp == 8){
                this->data_bitmap->palette() = BGRPalette::classic_332();
            }
        }
        else {
            this->data_bitmap = bmp.data_bitmap;
            this->data_bitmap->inc();
        }
    }

    Bitmap(uint8_t bpp, const BGRPalette * palette, uint16_t cx, uint16_t cy)
    : data_bitmap(DataBitmap::construct(bpp, cx,cy))
    {
        //LOG(LOG_INFO, "Creating bitmap (%p) cx=%u cy=%u size=%u bpp=%u", this, cx, cy, bmp_size, bpp);
        if (bpp == 8){
            if (palette){
                this->data_bitmap->palette() = *palette;
            }
            else {
                this->data_bitmap->palette() = BGRPalette::classic_332();
            }
        }

        if (this->cx() <= 0 || this->cy() <= 0) {
            LOG(LOG_ERR, "Bogus empty bitmap!!! cx=%u cy=%u bpp=%u", this->cx(), this->cy(), this->bpp());
        }
    }

private:
    void load_error_bitmap() {
        const uint8_t errorbmp[] = {
/* 0000 */ 0x00, 0x00, 0x00, 0x2d, 0x2d, 0xb6, 0x30, 0x30, 0xb8, 0x20, 0x20, 0x80, 0x07, 0x07, 0x33, 0x00,  // ...--.00.  ...3.
/* 0010 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ................
/* 0020 */ 0x00, 0x30, 0x30, 0xc8, 0x2c, 0x2c, 0xb8, 0x18, 0x18, 0x7f, 0x0c, 0x0c, 0x4d, 0x00, 0x00, 0x00,  // .00.,,......M...
/* 0030 */ 0x26, 0x26, 0xc3, 0x4c, 0x4c, 0xdd, 0x7a, 0x7a, 0xf2, 0x6f, 0x6f, 0xde, 0x43, 0x43, 0xa2, 0x00,  // &&.LL.zz.oo.CC..
/* 0040 */ 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x28,  // ..............((
/* 0050 */ 0xb4, 0x58, 0x58, 0xe1, 0x68, 0x68, 0xf4, 0x4d, 0x4d, 0xde, 0x26, 0x26, 0x9e, 0x07, 0x07, 0x5b,  // .XX.hh.MM.&&...[
/* 0060 */ 0x31, 0x31, 0xdb, 0x60, 0x60, 0xf5, 0x7c, 0x7c, 0xfe, 0x8e, 0x8e, 0xf9, 0x76, 0x76, 0xdd, 0x43,  // 11.``.||....vv.C
/* 0070 */ 0x43, 0xa5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x2d, 0xbb, 0x63, 0x63,  // C..........--.cc
/* 0080 */ 0xe0, 0x70, 0x70, 0xf6, 0x59, 0x59, 0xff, 0x42, 0x42, 0xf7, 0x36, 0x36, 0xde, 0x10, 0x10, 0x85,  // .pp.YY.BB.66....
/* 0090 */ 0x39, 0x39, 0xf3, 0x59, 0x59, 0xfc, 0x6f, 0x6f, 0xfe, 0x8c, 0x8c, 0xff, 0x97, 0x97, 0xf9, 0x78,  // 99.YY.oo.......x
/* 00a0 */ 0x78, 0xdd, 0x3c, 0x3c, 0xa0, 0x00, 0x00, 0x00, 0x2c, 0x2c, 0xc1, 0x5f, 0x5f, 0xdf, 0x79, 0x79,  // x.<<....,,.__.yy
/* 00b0 */ 0xf5, 0x6a, 0x6a, 0xfe, 0x47, 0x47, 0xff, 0x36, 0x36, 0xfd, 0x36, 0x36, 0xf6, 0x15, 0x15, 0xd0,  // .jj.GG.66.66....
/* 00c0 */ 0x37, 0x37, 0xfc, 0x46, 0x46, 0xf8, 0x60, 0x60, 0xfb, 0x7a, 0x7a, 0xfe, 0x92, 0x92, 0xfe, 0x8f,  // 77.FF.``.zz.....
/* 00d0 */ 0x8f, 0xf7, 0x6d, 0x6d, 0xda, 0x39, 0x39, 0xaf, 0x57, 0x57, 0xd5, 0x78, 0x78, 0xf5, 0x74, 0x74,  // ..mm.99.WW.xx.tt
/* 00e0 */ 0xfd, 0x59, 0x59, 0xfe, 0x43, 0x43, 0xfd, 0x3a, 0x3a, 0xf7, 0x2b, 0x2b, 0xf1, 0x14, 0x14, 0xf8,  // .YY.CC.::.++....
/* 00f0 */ 0x00, 0x00, 0x00, 0x34, 0x34, 0xf1, 0x4c, 0x4c, 0xf5, 0x62, 0x62, 0xfb, 0x78, 0x78, 0xfe, 0x87,  // ...44.LL.bb.xx..
/* 0100 */ 0x87, 0xfe, 0x7b, 0x7b, 0xf5, 0x65, 0x65, 0xe7, 0x6b, 0x6b, 0xef, 0x74, 0x74, 0xfe, 0x5f, 0x5f,  // ..{{.ee.kk.tt.__
/* 0110 */ 0xff, 0x4b, 0x4b, 0xfc, 0x3f, 0x3f, 0xf6, 0x34, 0x34, 0xea, 0x12, 0x12, 0xb4, 0x00, 0x00, 0x00,  // .KK.??.44.......
/* 0120 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x31, 0xed, 0x46, 0x46, 0xf3, 0x5a, 0x5a, 0xfb, 0x68,  // ......11.FF.ZZ.h
/* 0130 */ 0x68, 0xff, 0x6c, 0x6c, 0xfe, 0x6a, 0x6a, 0xfd, 0x64, 0x64, 0xfd, 0x59, 0x59, 0xff, 0x4a, 0x4a,  // h.ll.jj.dd.YY.JJ
/* 0140 */ 0xfd, 0x3f, 0x3f, 0xf6, 0x30, 0x30, 0xe8, 0x00, 0x00, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // .??.00..........
/* 0150 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x2c, 0xe7, 0x39, 0x39, 0xea, 0x48,  // .........,,.99.H
/* 0160 */ 0x48, 0xf7, 0x54, 0x54, 0xff, 0x52, 0x52, 0xff, 0x47, 0x47, 0xfe, 0x3a, 0x3a, 0xfb, 0x33, 0x33,  // H.TT.RR.GG.::.33
/* 0170 */ 0xec, 0x2d, 0x2d, 0xd6, 0x06, 0x06, 0xe7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // .--.............
/* 0180 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x15, 0x15, 0xdc, 0x37,  // ...............7
/* 0190 */ 0x37, 0xf5, 0x4d, 0x4d, 0xff, 0x4e, 0x4e, 0xff, 0x40, 0x40, 0xfe, 0x2b, 0x2b, 0xfa, 0x14, 0x14,  // 7.MM.NN.@@.++...
/* 01a0 */ 0xe0, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ................
/* 01b0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 0x04, 0x04, 0xd0, 0x28, 0x28, 0xee, 0x4c,  // ............((.L
/* 01c0 */ 0x4c, 0xfc, 0x55, 0x55, 0xff, 0x48, 0x48, 0xfe, 0x47, 0x47, 0xff, 0x42, 0x42, 0xfe, 0x25, 0x25,  // L.UU.HH.GG.BB.%%
/* 01d0 */ 0xf2, 0x0e, 0x0e, 0xd2, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ................
/* 01e0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x04, 0x04, 0xd0, 0x31, 0x31, 0xef, 0x61, 0x61, 0xfc, 0x70,  // .........11.aa.p
/* 01f0 */ 0x70, 0xff, 0x49, 0x49, 0xfa, 0x1f, 0x1f, 0xf3, 0x32, 0x32, 0xf7, 0x5f, 0x5f, 0xfe, 0x68, 0x68,  // p.II....22.__.hh
/* 0200 */ 0xff, 0x44, 0x44, 0xf4, 0x16, 0x16, 0xd3, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // .DD.............
/* 0210 */ 0x02, 0x02, 0x9a, 0x11, 0x11, 0xcb, 0x3b, 0x3b, 0xef, 0x7e, 0x7e, 0xfd, 0x94, 0x94, 0xfe, 0x65,  // ......;;.~~....e
/* 0220 */ 0x65, 0xfa, 0x27, 0x27, 0xee, 0x03, 0x03, 0xdb, 0x13, 0x13, 0xe6, 0x49, 0x49, 0xf7, 0x86, 0x86,  // e.''.......II...
/* 0230 */ 0xfe, 0x8f, 0x8f, 0xfe, 0x5c, 0x5c, 0xf5, 0x1c, 0x1c, 0xd0, 0x03, 0x03, 0x90, 0x00, 0x00, 0x36,  // ...............6
/* 0240 */ 0x06, 0x06, 0xbe, 0x40, 0x40, 0xef, 0x9e, 0x9e, 0xfe, 0xbb, 0xbb, 0xff, 0x83, 0x83, 0xfb, 0x33,  // ...@@..........3
/* 0250 */ 0x33, 0xee, 0x00, 0x00, 0xd5, 0x00, 0x00, 0x57, 0x00, 0x00, 0xce, 0x18, 0x18, 0xe7, 0x5f, 0x5f,  // 3......W......__
/* 0260 */ 0xf7, 0xaa, 0xaa, 0xfe, 0xb7, 0xb7, 0xff, 0x6d, 0x6d, 0xf5, 0x13, 0x13, 0xcf, 0x00, 0x00, 0x68,  // .......mm......h
/* 0270 */ 0x09, 0x09, 0xd7, 0x60, 0x60, 0xf4, 0xbe, 0xbe, 0xff, 0x9e, 0x9e, 0xfb, 0x3a, 0x3a, 0xed, 0x00,  // ...``.......::..
/* 0280 */ 0x00, 0xd3, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce, 0x20, 0x20,  // ..............
/* 0290 */ 0xe6, 0x73, 0x73, 0xf6, 0xc6, 0xc6, 0xff, 0x94, 0x94, 0xfa, 0x22, 0x22, 0xe7, 0x01, 0x01, 0xa5,  // .ss.......""....
/* 02a0 */ 0x04, 0x04, 0xcd, 0x25, 0x25, 0xe1, 0x60, 0x60, 0xf3, 0x37, 0x37, 0xeb, 0x0b, 0x0b, 0xcf, 0x00,  // ...%%.``.77.....
/* 02b0 */ 0x00, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // .y..............
/* 02c0 */ 0xc8, 0x32, 0x32, 0xe4, 0x61, 0x61, 0xf6, 0x37, 0x37, 0xeb, 0x11, 0x11, 0xce, 0x00, 0x00, 0xad,  // .22.aa.77.......
/* 02d0 */ 0x00, 0x00, 0x00, 0x05, 0x05, 0xc6, 0x0a, 0x0a, 0xd0, 0x06, 0x06, 0xbf, 0x00, 0x00, 0x8c, 0x00,  // ................
/* 02e0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ................
/* 02f0 */ 0x00, 0x08, 0x08, 0xcf, 0x0a, 0x0a, 0xcf, 0x06, 0x06, 0xbe, 0x02, 0x02, 0xa9, 0x00, 0x00, 0x00  // ................
        };

        REDASSERT(!this->data_bitmap);

        this->data_bitmap = DataBitmap::construct(24, 16, 16);
        memcpy(this->data_bitmap->get(), errorbmp, sizeof(errorbmp));
    }
};

inline void swap(Bitmap & a, Bitmap & b) noexcept {
    a.swap(b);
}

#endif

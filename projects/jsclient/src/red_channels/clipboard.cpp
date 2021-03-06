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

#include "red_channels/clipboard.hpp"

#include "red_emscripten/em_asm.hpp"
#include "red_emscripten/val.hpp"

#include "utils/log.hpp"
#include "utils/sugar/overload.hpp"

#include "core/callback.hpp"

#include "core/RDP/clipboard.hpp"


namespace redjs::channels::clipboard
{

namespace
{
    constexpr int first_last_channel_flags
      = CHANNELS::CHANNEL_FLAG_LAST
      | CHANNELS::CHANNEL_FLAG_FIRST
      | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL
    ;

    bytes_view quick_utf16_av(bytes_view av)
    {
        auto first = av.begin();
        auto count = av.size() / 2;

        while (count > 0)
        {
            auto it = first;
            auto step = count / 2;
            it += step * 2;
            if (it[0] | it[1])
            {
                first = it + 2;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }

        return av = av.first(first - av.begin());
    }

    namespace constants
    {
        namespace file_group_descriptor_w
        {
            // flags(4) + reserved1(32) + fileAttributes(4) + reserved2(16) + lastWriteTime(8)
            constexpr std::size_t useless_attributes_size = 64;
            // sizeHigh(4) + sizeLow(4)
            constexpr std::size_t filesize_attribute_size = 8;
            // filenameUtf16(520)
            constexpr std::size_t filename_attribute_size = 520;
            constexpr std::size_t file_packet_size
                = useless_attributes_size
                + filename_attribute_size
                + filesize_attribute_size;
        }
    }

}


std::size_t FormatListEmptyName::size() const noexcept
{
    return
        4 /*FormatId*/
    // empty name
        + (this->use_long_format_names ? 2 /* unicode null character*/ : 32);
}

void FormatListEmptyName::emit(ClipboardFormat cf, OutStream& out_stream) noexcept
{
    out_stream.out_uint32_le(underlying_cast(cf));

    if (this->use_long_format_names)
    {
        out_stream.out_uint8(0);
        out_stream.out_uint8(0);
    }
    else
    {
        char buffer[32]{};
        out_stream.out_copy_bytes(make_array_view(buffer));
    }
}


ClipboardChannel::ClipboardChannel(Callback& cb, emscripten::val&& callbacks, RDPVerbose verbose)
: cb(cb)
, callbacks(std::move(callbacks))
, verbose(bool(verbose & RDPVerbose::cliprdr))
{}

ClipboardChannel::~ClipboardChannel() = default;

void ClipboardChannel::send_file_contents_request(
    uint32_t request_type,
    uint32_t stream_id, uint32_t lindex,
    uint32_t pos_low, uint32_t pos_high)
{
    StaticOutStream<64> out_stream;

    RDPECLIP::FileContentsRequestPDU request(
        stream_id,
        lindex,
        request_type,
        pos_low,
        pos_high,
        request_type == RDPECLIP::FILECONTENTS_SIZE
        ? RDPECLIP::FILECONTENTS_SIZE_CB_REQUESTED
        : 0x0000ffff // TODO maximum number of bytes to read from the remote file
    );

    RDPECLIP::CliprdrHeader header(
        RDPECLIP::CB_FILECONTENTS_REQUEST,
        RDPECLIP::CB_RESPONSE__NONE_,
        request.size());

    header.emit(out_stream);
    request.emit(out_stream);
    this->send_data(out_stream.get_bytes());

    this->custom_cf = (request_type == RDPECLIP::FILECONTENTS_SIZE)
        ? CustomFormat::FileContentsSize
        : CustomFormat::FileContentsRange;
}

void ClipboardChannel::send_request_format(uint32_t format_id, CustomFormat custom_cf)
{
    LOG_IF(this->verbose, LOG_INFO,
        "Clipboard: Send Request Format id=%d custom=%d", format_id, custom_cf);

    RDPECLIP::CliprdrHeader formatListRequestPDUHeader(RDPECLIP::CB_FORMAT_DATA_REQUEST, RDPECLIP::CB_RESPONSE__NONE_, 4);
    RDPECLIP::FormatDataRequestPDU formatDataRequestPDU(format_id);
    StaticOutStream<256> out_stream;
    formatListRequestPDUHeader.emit(out_stream);
    formatDataRequestPDU.emit(out_stream);
    InStream chunkRequest(out_stream.get_bytes());
    this->requested_format_id = format_id;
    this->custom_cf = custom_cf;

    this->send_data(out_stream.get_bytes());
}

void ClipboardChannel::receive(bytes_view data, int channel_flags)
{
    if (this->wating_for_data)
    {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Response PDU Continuation");
        this->process_format_data_response(data, channel_flags, 0);
        return ;
    }

    InStream chunk(data);
    RDPECLIP::CliprdrHeader header;
    header.recv(chunk);

    if (header.msgFlags() == RDPECLIP::CB_RESPONSE_FAIL)
    {
        LOG(LOG_WARNING, "Clipboard: Format List Response PDU");
        return ;
    }

    switch (header.msgType())
    {
    case RDPECLIP::CB_CLIP_CAPS:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Capabilities PDU");
        this->process_capabilities(chunk);
        break;

    case RDPECLIP::CB_MONITOR_READY:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Monitor Ready PDU");
        this->process_monitor_ready();
        break;

    case RDPECLIP::CB_FORMAT_LIST_RESPONSE:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format List Response PDU");
        break;

    case RDPECLIP::CB_FORMAT_LIST:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format List PDU");

        LOG_IF(header.dataLen() > chunk.in_remain(), LOG_WARNING,
            "Server Format List PDU data length(%u) longer than chunk(%zu)",
            header.dataLen(), chunk.in_remain());

        this->process_format_list(chunk, channel_flags);
        break;

    case RDPECLIP::CB_FORMAT_DATA_RESPONSE:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format Data Response PDU");
        this->process_format_data_response(chunk.remaining_bytes(), channel_flags, header.dataLen());
    break;

    case RDPECLIP::CB_FILECONTENTS_RESPONSE: {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Response PDU");
        this->process_format_data_response(chunk.remaining_bytes(), channel_flags, header.dataLen());
    }
    break;

    case RDPECLIP::CB_FORMAT_DATA_REQUEST:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Format Data Request PDU");
        this->process_format_data_request(chunk);
    break;

    case RDPECLIP::CB_FILECONTENTS_REQUEST:
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Request PDU");

        this->process_filecontents_request(chunk);
    break;

    // case RDPECLIP::CB_LOCK_CLIPDATA:
    //     LOG_IF(bool(this->verbose & RDPVerbose::cliprdr), LOG_INFO,
    //         "SERVER >> CB Channel: Lock Clipboard Data PDU");
    // break;

    // case RDPECLIP::CB_UNLOCK_CLIPDATA:
    //     LOG_IF(bool(this->verbose & RDPVerbose::cliprdr), LOG_INFO,
    //         "SERVER >> CB Channel: Unlock Clipboard Data PDU");
    // break;

    default:
        LOG_IF(this->verbose, LOG_ERR,
            "Clipboard: Default Process server PDU data (%" PRIu16 ")", header.msgType());
        // this->process_server_clipboard_indata(flags, chunk, this->_cb_buffers, this->_cb_filesList);
        break;

    }
}

void ClipboardChannel::process_format_data_request(InStream& chunk)
{
    auto format_id = chunk.in_uint32_le();
    emval_call(this->callbacks, "receiveFormatId", format_id);
}

void ClipboardChannel::process_filecontents_request(InStream& chunk)
{
    auto stream_id = chunk.in_uint32_le();
    auto lindex = chunk.in_sint32_le();
    auto type = chunk.in_uint32_le();
    auto npos_low = chunk.in_uint32_le();
    auto npos_high = chunk.in_uint32_le();
    auto cb_requested = chunk.in_uint32_le();
    // auto clip_data_id = chunk.in_uint32_le(); // optional
    emval_call(this->callbacks, "receiveFileContentsRequest",
        stream_id, type, lindex, npos_low, npos_high, cb_requested);
}

namespace
{
    struct Utf8AsUtf16
    {
        bytes_view _utf8_bytes;
        std::size_t size() const noexcept { return _utf8_bytes.size(); }
        uint8_t const* data() const noexcept { return _utf8_bytes.data(); }
    };

    // This is a hack
    struct FormatNameUtf8AsUtf16
    {
        uint32_t _format_id;
        Utf8AsUtf16 _bytes;

        uint32_t format_id() const noexcept { return this->_format_id; }
        Utf8AsUtf16 utf8_name() const noexcept { return this->_bytes; }
    };

    std::size_t UTF8toUTF16(Utf8AsUtf16 const& utf16, writable_bytes_view target) noexcept
    {
        auto len = std::min(utf16._utf8_bytes.size(), target.size());
        len = len - (len & 1u);
        memcpy(target.as_charp(), utf16._utf8_bytes.data(), len);
        return len;
    }

    bool is_ASCII_string(Utf8AsUtf16 const&) noexcept
    {
        return false;
    }
}

unsigned ClipboardChannel::add_format(writable_bytes_view data, uint32_t format_id, Charset charset, bytes_view name)
{
    Cliprdr::FormatNameRef format{format_id, name};
    OutStream out_stream(data);

    switch (charset)
    {
        case Charset::Ascii:
            if (this->format_list.use_long_format_names)
            {
                Cliprdr::format_list_serialize_long_format(out_stream, format);
            }
            else
            {
                Cliprdr::format_list_serialize_ascii_format(out_stream, format);
            }
            break;

        case Charset::Utf16:
            if (this->format_list.use_long_format_names)
            {
                Cliprdr::format_list_serialize_long_format(out_stream,
                    FormatNameUtf8AsUtf16{format_id, {name}});
            }
            else
            {
                Cliprdr::format_list_serialize_unicode_format(out_stream, format);
            }
    }

    return out_stream.get_offset();
}

void ClipboardChannel::send_format(uint32_t format_id, Charset charset, bytes_view name)
{
    StaticOutStream<128> out_stream;

    switch (charset)
    {
        case Charset::Ascii:
            Cliprdr::format_list_serialize_with_header(
                out_stream,
                Cliprdr::IsLongFormat(this->format_list.use_long_format_names),
                std::array<FormatNameUtf8AsUtf16, 1>{{{format_id, {name}}}});
            break;

        case Charset::Utf16:
            Cliprdr::format_list_serialize_with_header(
                out_stream,
                Cliprdr::IsLongFormat(this->format_list.use_long_format_names),
                std::array<Cliprdr::FormatNameRef, 1>{{{format_id, name}}});
    }

    this->send_data(out_stream.get_bytes());
}

void ClipboardChannel::send_header(uint16_t type, uint16_t flags, uint32_t total_data_len, uint32_t channel_flags)
{
    StaticOutStream<12> out_stream;

    RDPECLIP::CliprdrHeader header(type, flags, total_data_len);
    header.emit(out_stream);

    this->send_data(out_stream.get_bytes(), total_data_len + out_stream.get_offset(),
        channel_flags | CHANNELS::CHANNEL_FLAG_FIRST);
}

void ClipboardChannel::send_data(bytes_view data, uint32_t total_data_len, uint32_t channel_flags)
{
    InStream in_stream(data);
    this->cb.send_to_mod_channel(
        channel_names::cliprdr,
        in_stream,
        total_data_len,
        channel_flags | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL
    );
}

void ClipboardChannel::send_data(bytes_view av)
{
    this->send_data(av, av.size(), first_last_channel_flags);
}

void ClipboardChannel::process_format_data_response(bytes_view data, uint32_t channel_flags, uint32_t data_len)
{
    const bool is_first_packet = (channel_flags & CHANNELS::CHANNEL_FLAG_FIRST);
    const bool is_last_packet = (channel_flags & CHANNELS::CHANNEL_FLAG_LAST);

    if (is_first_packet)
    {
        this->response_buffer.clear();
        this->data_len = data_len;
    }

    this->wating_for_data = !(channel_flags & CHANNELS::CHANNEL_FLAG_LAST);

    auto send_data = [&](char const* fname, bytes_view data, auto const&... args){
        emval_call(this->callbacks, fname, data.data(), data.size(), args...);
    };

    auto send_data2 = [&](char const* fname, auto const&... args){
        emval_call(this->callbacks, fname, args...);
    };

    auto remove_last_char = [](bytes_view data, std::size_t strip_n) {
        return data.size() >= strip_n ? data.first(data.size() - strip_n) : bytes_view{};
    };

    if (data.size() > this->data_len)
    {
        data = data.first(this->data_len);
    }
    this->data_len -= data.size();

    switch (this->custom_cf)
    {
    case CustomFormat::None: {
        auto receive_data = [&](bytes_view av, int flags){
            if (!av.empty())
            {
                send_data("receiveData", av, this->requested_format_id, flags);
            }
        };

        switch (this->requested_format_id)
        {
        case RDPECLIP::CF_TEXT:
            if (is_last_packet)
            {
                data = remove_last_char(data, 1);
            }
            break;

        case RDPECLIP::CF_UNICODETEXT:
            if (is_last_packet)
            {
                if (is_first_packet)
                {
                    data = remove_last_char(data, 2);
                }
                else if (data.size() > 2)
                {
                    receive_data(this->response_buffer.as_bytes(), 0);
                    data = data.first(data.size()-2u);
                }
                else if (data.size() == 2)
                {
                    receive_data(this->response_buffer.as_bytes(), CHANNELS::CHANNEL_FLAG_LAST);
                    data = {};
                }
                else if (data.size() == 1)
                {
                    data = {};
                }
            }
            else
            {
                receive_data(this->response_buffer.as_bytes(), 0);

                if (data.empty())
                {
                    this->response_buffer.size = 0;
                }
                else
                {
                    this->response_buffer.set(data.last(1));
                    data = data.first(data.size()-1u);
                }
            }
            break;
        }

        receive_data(data, channel_flags & (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST));
        break;
    }

    case CustomFormat::FileGroupDescriptorW: {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Group Descriptor Response PDU");

        InStream in_stream(data);

        if (is_first_packet)
        {
            auto nb_item = in_stream.in_uint32_le();
            send_data2("receiveNbFileName", nb_item);
        }

        namespace constants = constants::file_group_descriptor_w;

        auto extract_file = [&](InStream& in_stream){
            auto flags = in_stream.in_uint32_le();
            in_stream.in_skip_bytes(32);
            auto file_attrs = in_stream.in_uint32_le();
            in_stream.in_skip_bytes(16);
            auto last_write_time_high = in_stream.in_uint32_le();
            auto last_write_time_low = in_stream.in_uint32_le();
            auto size_high = in_stream.in_uint32_le();
            auto size_low = in_stream.in_uint32_le();
            auto name = quick_utf16_av(in_stream.remaining_bytes()
                .first(constants::filename_attribute_size));
            send_data("receiveFileName", name, file_attrs, flags, size_low, size_high, last_write_time_low, last_write_time_high);
        };

        if (this->response_buffer.size
            && in_stream.in_remain() + this->response_buffer.size >= constants::file_packet_size)
        {
            auto nbcopy = constants::file_packet_size - this->response_buffer.size;
            this->response_buffer.add(in_stream.remaining_bytes().first(nbcopy));

            InStream in_stream(this->response_buffer.as_bytes());
            extract_file(in_stream);
            this->response_buffer.clear();
            assert(in_stream.in_remain() == 0);
        }

        while (in_stream.in_remain() >= constants::file_packet_size)
        {
            extract_file(in_stream);
        }

        this->response_buffer.add(in_stream.remaining_bytes());
        break;
    }

    case CustomFormat::FileContentsSize: {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Response PDU SIZE");

        InStream in_stream(data);
        auto stream_id = in_stream.in_uint32_le();
        auto size_low = in_stream.in_uint32_le();
        auto size_high = in_stream.in_uint32_le();

        send_data2("receiveFileSize", size_high, size_low, stream_id);
        break;
    }

    case CustomFormat::FileContentsRange: {
        LOG_IF(this->verbose, LOG_INFO, "Clipboard: File Contents Response PDU RANGE");

        InStream in_stream(data);

        if (is_first_packet)
        {
            this->response_buffer.set(in_stream.remaining_bytes().first(4));
        }

        uint32_t stream_id = Parse(this->response_buffer.data.data()).in_uint32_le();

        send_data("receiveFileContents", in_stream.remaining_bytes(), stream_id,
            channel_flags & (CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST));
        break;
    }
    }
}

void ClipboardChannel::process_format_list(InStream& chunk, uint32_t channel_flags)
{
    emval_call(this->callbacks, "receiveFormatStart");

    Cliprdr::format_list_extract(
        chunk,
        Cliprdr::IsLongFormat(this->format_list.use_long_format_names),
        Cliprdr::IsAscii(channel_flags & RDPECLIP::CB_ASCII_NAMES),
        [&](uint32_t format_id, auto name){
            auto av_name = name.bytes;
            bool is_utf8 = true;

            if (av_name.empty())
            {
                switch (format_id)
                {
                case RDPECLIP::CF_TEXT:            av_name = "text"_av; break;
                case RDPECLIP::CF_BITMAP:          av_name = "bitmap"_av; break;
                case RDPECLIP::CF_METAFILEPICT:    av_name = "metafilepict"_av; break;
                case RDPECLIP::CF_SYLK:            av_name = "sylk"_av; break;
                case RDPECLIP::CF_DIF:             av_name = "dif"_av; break;
                case RDPECLIP::CF_TIFF:            av_name = "tiff"_av; break;
                case RDPECLIP::CF_OEMTEXT:         av_name = "oemtext"_av; break;
                case RDPECLIP::CF_DIB:             av_name = "dib"_av; break;
                case RDPECLIP::CF_PALETTE:         av_name = "palette"_av; break;
                case RDPECLIP::CF_PENDATA:         av_name = "pendata"_av; break;
                case RDPECLIP::CF_RIFF:            av_name = "riff"_av; break;
                case RDPECLIP::CF_WAVE:            av_name = "wave"_av; break;
                case RDPECLIP::CF_UNICODETEXT:     av_name = "unicodetext"_av; break;
                case RDPECLIP::CF_ENHMETAFILE:     av_name = "enhmetafile"_av; break;
                case RDPECLIP::CF_HDROP:           av_name = "hdrop"_av; break;
                case RDPECLIP::CF_LOCALE:          av_name = "locale"_av; break;
                case RDPECLIP::CF_DIBV5:           av_name = "dibv5"_av; break;
                case RDPECLIP::CF_OWNERDISPLAY:    av_name = "ownerdisplay"_av; break;
                case RDPECLIP::CF_DSPTEXT:         av_name = "dsptext"_av; break;
                case RDPECLIP::CF_DSPBITMAP:       av_name = "dspbitmap"_av; break;
                case RDPECLIP::CF_DSPMETAFILEPICT: av_name = "dspmetafilepict"_av; break;
                case RDPECLIP::CF_DSPENHMETAFILE:  av_name = "dspenhmetafile"_av; break;
                case RDPECLIP::CF_PRIVATEFIRST:    av_name = "privatefirst"_av; break;
                case RDPECLIP::CF_PRIVATELAST:     av_name = "privatelast"_av; break;
                case RDPECLIP::CF_GDIOBJFIRST:     av_name = "gdiobjfirst"_av; break;
                case RDPECLIP::CF_GDIOBJLAST:      av_name = "gdiobjlast"_av; break;
                }
            }
            else {
                is_utf8 = overload{
                    [](Cliprdr::AsciiName const&) { return true; },
                    [&](Cliprdr::UnicodeName const&) { return false; },
                }(name);
            }

            emval_call(this->callbacks, "receiveFormat",
                av_name.data(), av_name.size(), format_id, is_utf8);
        }
    );

    this->send_format_list_response_ok();

    emval_call(this->callbacks, "receiveFormatStop");
}

void ClipboardChannel::send_format_list_response_ok()
{
    StaticOutStream<256> out_stream;
    RDPECLIP::CliprdrHeader formatListResponsePDUHeader(
        RDPECLIP::CB_FORMAT_LIST_RESPONSE, RDPECLIP::CB_RESPONSE_OK, 0);
    formatListResponsePDUHeader.emit(out_stream);
    this->send_data(out_stream.get_bytes());
}

void ClipboardChannel::process_capabilities(InStream& chunk)
{
    RDPECLIP::ClipboardCapabilitiesPDU pdu;
    pdu.recv(chunk);

    RDPECLIP::GeneralCapabilitySet pdu2;
    pdu2.recv(chunk);

    this->format_list.use_long_format_names
        = bool(pdu2.generalFlags() & RDPECLIP::CB_USE_LONG_FORMAT_NAMES);
}

void ClipboardChannel::process_monitor_ready()
{
    {
        uint16_t cCapabilitiesSets = 1;
        uint32_t const generalFlags
            = (this->format_list.use_long_format_names ? RDPECLIP::CB_USE_LONG_FORMAT_NAMES : 0)
            | RDPECLIP::CB_STREAM_FILECLIP_ENABLED
            | RDPECLIP::CB_FILECLIP_NO_FILE_PATHS
            | RDPECLIP::CB_HUGE_FILE_SUPPORT_ENABLED
            // | RDPECLIP::CB_CAN_LOCK_CLIPDATA
        ;

        RDPECLIP::ClipboardCapabilitiesPDU clipboard_caps_pdu(cCapabilitiesSets);
        RDPECLIP::GeneralCapabilitySet general_cap_set(RDPECLIP::CB_CAPS_VERSION_2, generalFlags);
        RDPECLIP::CliprdrHeader header(RDPECLIP::CB_CLIP_CAPS, RDPECLIP::CB_RESPONSE__NONE_,
            clipboard_caps_pdu.size() + general_cap_set.size());

        StaticOutStream<1024> out_stream;
        header.emit(out_stream);
        clipboard_caps_pdu.emit(out_stream);
        general_cap_set.emit(out_stream);

        this->send_data(out_stream.get_bytes());

        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Send Capabilities PDU");
    }

    {
        RDPECLIP::CliprdrHeader header(
            RDPECLIP::CB_FORMAT_LIST,
            RDPECLIP::CB_ASCII_NAMES,
            format_list.size());

        StaticOutStream<1600> out_stream;

        header.emit(out_stream);
        this->format_list.emit(ClipboardFormat::UnicodeText, out_stream);

        this->send_data(out_stream.get_bytes());

        LOG_IF(this->verbose, LOG_INFO, "Clipboard: Send Format List PDU");
    }
}


void ClipboardChannel::ResponseBuffer::set(bytes_view av)
{
    assert(av.size() <= this->data.size());
    memcpy(this->data.data(), av.data(), av.size());
    this->size = av.size();
}

void ClipboardChannel::ResponseBuffer::add(bytes_view av)
{
    assert(av.size() + this->size <= this->data.size());
    memcpy(this->data.data() + this->size, av.data(), av.size());
    this->size += av.size();
}

bytes_view ClipboardChannel::ResponseBuffer::as_bytes() const
{
    return {this->data.data(), this->size};
}

} // namespace redjs::channel::clipboard

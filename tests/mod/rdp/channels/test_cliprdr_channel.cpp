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

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2015
    Author(s): Christophe Grosjean, Raphael Zhou
*/


#include "test_only/test_framework/redemption_unit_tests.hpp"
#include "test_only/test_framework/working_directory.hpp"
#include "test_only/test_framework/file.hpp"

#include "test_only/transport/test_transport.hpp"
#include "test_only/fake_stat.hpp"
#include "test_only/lcg_random.hpp"
#include "mod/rdp/channels/cliprdr_channel.hpp"
#include "core/RDP/clipboard.hpp"

#include "./test_channel.hpp"

namespace
{
    namespace data_full_auth
    {
        #include "fixtures/test_cliprdr_channel_xfreerdp_full_authorisation.hpp"
        const auto cb_params = []{ /*NOLINT*/
            ClipboardVirtualChannelParams clipboard_virtual_channel_params;
            clipboard_virtual_channel_params.clipboard_up_authorized   = true;
            clipboard_virtual_channel_params.clipboard_down_authorized = true;
            clipboard_virtual_channel_params.clipboard_file_authorized = true;
            return clipboard_virtual_channel_params;
        }();
    } // namespace data_full_auth

    namespace data_down_denied
    {
        #include "fixtures/test_cliprdr_channel_xfreerdp_down_denied.hpp"
        const auto cb_params = []{ /*NOLINT*/
            ClipboardVirtualChannelParams clipboard_virtual_channel_params;
            clipboard_virtual_channel_params.clipboard_up_authorized   = true;
            clipboard_virtual_channel_params.clipboard_down_authorized = false;
            clipboard_virtual_channel_params.clipboard_file_authorized = true;
            return clipboard_virtual_channel_params;
        }();
    } // namespace data_down_denied

    namespace data_up_denied
    {
        #include "fixtures/test_cliprdr_channel_xfreerdp_up_denied.hpp"
        const auto cb_params = []{ /*NOLINT*/
            ClipboardVirtualChannelParams clipboard_virtual_channel_params;
            clipboard_virtual_channel_params.clipboard_up_authorized   = false;
            clipboard_virtual_channel_params.clipboard_down_authorized = true;
            clipboard_virtual_channel_params.clipboard_file_authorized = true;
            return clipboard_virtual_channel_params;
        }();
    } // namespace data_up_denied

    namespace data_full_denied
    {
        #include "fixtures/test_cliprdr_channel_xfreerdp_full_denied.hpp"
        const auto cb_params = []{ /*NOLINT*/
            ClipboardVirtualChannelParams clipboard_virtual_channel_params;
            clipboard_virtual_channel_params.clipboard_up_authorized   = false;
            clipboard_virtual_channel_params.clipboard_down_authorized = false;
            clipboard_virtual_channel_params.clipboard_file_authorized = true;
            return clipboard_virtual_channel_params;
        }();
    } // namespace data_full_denied
} // anonymous namespace

RED_AUTO_TEST_CASE(TestCliprdrChannelXfreeRDPAuthorisation)
{
    NullReportMessage report_message;
    FileValidatorService * ipca_service = nullptr;

    BaseVirtualChannel::Params base_params(report_message, RDPVerbose::cliprdr | RDPVerbose::cliprdr_dump);

    struct D
    {
        char const* name;
        ClipboardVirtualChannelParams cb_params;
        bytes_view indata;
        bytes_view outdata;
    };

    #define F(name) D{#name,           \
        name::cb_params,               \
        cstr_array_view(name::indata), \
        cstr_array_view(name::outdata) \
    }

    RED_TEST_CONTEXT_DATA(D const& d, d.name, {
        F(data_full_auth),
        F(data_down_denied),
        F(data_up_denied),
        F(data_full_denied)
    })
    {
        TestTransport t(d.indata, d.outdata);

        TestToClientSender to_client_sender(t);
        TestToServerSender to_server_sender(t);

        SessionReactor session_reactor;

        ClipboardVirtualChannel clipboard_virtual_channel(
            &to_client_sender, &to_server_sender, session_reactor,
            base_params, d.cb_params, ipca_service, {nullptr, false});

        RED_CHECK_EXCEPTION_ERROR_ID(
            CHECK_CHANNEL(t, clipboard_virtual_channel),
            ERR_TRANSPORT_NO_MORE_DATA);
    }
}


class NullSender : public VirtualChannelDataSender
{
public:
    void operator() (uint32_t /*total_length*/, uint32_t /*flags*/, bytes_view /*chunk_data*/) override {}
};

RED_AUTO_TEST_CASE(TestCliprdrChannelMalformedFormatListPDU)
{
    NullReportMessage report_message;
    FileValidatorService * ipca_service = nullptr;

    BaseVirtualChannel::Params base_params(report_message, RDPVerbose::cliprdr | RDPVerbose::cliprdr_dump);

    ClipboardVirtualChannelParams clipboard_virtual_channel_params;
    clipboard_virtual_channel_params.clipboard_down_authorized = true;
    clipboard_virtual_channel_params.clipboard_up_authorized   = true;
    clipboard_virtual_channel_params.clipboard_file_authorized = true;

    NullSender to_client_sender;
    NullSender to_server_sender;

    SessionReactor session_reactor;

    ClipboardVirtualChannel clipboard_virtual_channel(
        &to_client_sender, &to_server_sender, session_reactor,
        base_params, clipboard_virtual_channel_params, ipca_service, {nullptr, false});

    uint8_t  virtual_channel_data[CHANNELS::CHANNEL_CHUNK_LENGTH];
    InStream virtual_channel_stream(virtual_channel_data);

    StaticOutStream<256> out_s;
    Cliprdr::format_list_serialize_with_header(
        out_s, Cliprdr::IsLongFormat(true),
        std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, {}}});

    const size_t totalLength = out_s.get_offset();

    clipboard_virtual_channel.process_client_message(
            totalLength,
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
            out_s.get_bytes());
}

RED_AUTO_TEST_CASE(TestCliprdrChannelFailedFormatDataResponsePDU)
{
    NullReportMessage report_message;
    FileValidatorService * ipca_service = nullptr;

    BaseVirtualChannel::Params base_params(report_message, RDPVerbose::cliprdr | RDPVerbose::cliprdr_dump);

    ClipboardVirtualChannelParams clipboard_virtual_channel_params;
    clipboard_virtual_channel_params.clipboard_down_authorized = true;
    clipboard_virtual_channel_params.clipboard_up_authorized   = true;
    clipboard_virtual_channel_params.clipboard_file_authorized = true;

    NullSender to_client_sender;
    NullSender to_server_sender;

    SessionReactor session_reactor;

    ClipboardVirtualChannel clipboard_virtual_channel(
        &to_client_sender, &to_server_sender, session_reactor,
        base_params, clipboard_virtual_channel_params, ipca_service, {nullptr, false});

// ClipboardVirtualChannel::process_server_message: total_length=28 flags=0x00000003 chunk_data_length=28
// Recv done on channel (28) n bytes
// /* 0000 */ "\x01\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x1c\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x03\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x1c\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x07\x00\x00\x00\x10\x00\x00\x00\x01\x00\x00\x00\x01\x00\x0c\x00" // ................
// /* 0010 */ "\x02\x00\x00\x00\x1e\x00\x00\x00\x00\x00\x00\x00"                 // ............
// Dump done on channel (28) n bytes
// ClipboardVirtualChannel::process_server_message: Clipboard Capabilities PDU
// ClipboardVirtualChannel::process_server_clipboard_capabilities_pdu: General Capability Set
// RDPECLIP::GeneralCapabilitySet: capabilitySetType=CB_CAPSTYPE_GENERAL(1) lengthCapability=12 version=CB_CAPS_VERSION_2(0x00000002) generalFlags=0x0000001E

    std::unique_ptr<AsynchronousTask> out_asynchronous_task;

    clipboard_virtual_channel.process_server_message(
            28,
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
        /* 0000 */ "\x07\x00\x00\x00\x10\x00\x00\x00\x01\x00\x00\x00\x01\x00\x0c\x00" // ................
        /* 0010 */ "\x02\x00\x00\x00\x1e\x00\x00\x00\x00\x00\x00\x00"                 // ............
                ""_av,
            out_asynchronous_task);

// ClipboardVirtualChannel::process_client_message: total_length=24 flags=0x00000013 chunk_data_length=24
// Recv done on channel (24) n bytes
// /* 0000 */ "\x00\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x18\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x13\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x18\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x07\x00\x00\x00\x10\x00\x00\x00\x01\x00\x00\x00\x01\x00\x0c\x00" // ................
// /* 0010 */ "\x02\x00\x00\x00\x1e\x00\x00\x00"                                 // ........
// Dump done on channel (24) n bytes
// ClipboardVirtualChannel::process_client_message: Clipboard Capabilities PDU
// ClipboardVirtualChannel::process_client_clipboard_capabilities_pdu: General Capability Set
// RDPECLIP::GeneralCapabilitySet: capabilitySetType=CB_CAPSTYPE_GENERAL(1) lengthCapability=12 version=CB_CAPS_VERSION_2(0x00000002) generalFlags=0x0000001E

    clipboard_virtual_channel.process_client_message(
            24,
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
    /* 0000 */ "\x07\x00\x00\x00\x10\x00\x00\x00\x01\x00\x00\x00\x01\x00\x0c\x00" // ................
    /* 0010 */ "\x02\x00\x00\x00\x1e\x00\x00\x00"                                 // ........
                ""_av);

// ClipboardVirtualChannel::process_client_message: total_length=130 flags=0x00000013 chunk_data_length=130
// Recv done on channel (130) n bytes
// /* 0000 */ "\x00\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x82\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x13\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x82\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x02\x00\x00\x00\x7a\x00\x00\x00\x6e\xc0\x00\x00\x46\x00\x69\x00" // ....z...n...F.i.
// /* 0010 */ "\x6c\x00\x65\x00\x47\x00\x72\x00\x6f\x00\x75\x00\x70\x00\x44\x00" // l.e.G.r.o.u.p.D.
// /* 0020 */ "\x65\x00\x73\x00\x63\x00\x72\x00\x69\x00\x70\x00\x74\x00\x6f\x00" // e.s.c.r.i.p.t.o.
// /* 0030 */ "\x72\x00\x57\x00\x00\x00\x94\xc0\x00\x00\x46\x00\x69\x00\x6c\x00" // r.W.......F.i.l.
// /* 0040 */ "\x65\x00\x43\x00\x6f\x00\x6e\x00\x74\x00\x65\x00\x6e\x00\x74\x00" // e.C.o.n.t.e.n.t.
// /* 0050 */ "\x73\x00\x00\x00\x10\xc1\x00\x00\x50\x00\x72\x00\x65\x00\x66\x00" // s.......P.r.e.f.
// /* 0060 */ "\x65\x00\x72\x00\x72\x00\x65\x00\x64\x00\x20\x00\x44\x00\x72\x00" // e.r.r.e.d. .D.r.
// /* 0070 */ "\x6f\x00\x70\x00\x45\x00\x66\x00\x66\x00\x65\x00\x63\x00\x74\x00" // o.p.E.f.f.e.c.t.
// /* 0080 */ "\x00\x00"                                                         // ..
// Dump done on channel (130) n bytes
// ClipboardVirtualChannel::process_client_message: Format List PDU
// ClipboardVirtualChannel::process_client_format_list_pdu: Long Format Name variant of Format List PDU is used for exchanging updated format names.
// ClipboardVirtualChannel::process_client_format_list_pdu: formatId=<unknown>(49262) wszFormatName="FileGroupDescriptorW"
// ClipboardVirtualChannel::process_client_format_list_pdu: formatId=<unknown>(49300) wszFormatName="FileContents"
// ClipboardVirtualChannel::process_client_format_list_pdu: formatId=<unknown>(49424) wszFormatName="Preferred DropEffect"

    clipboard_virtual_channel.process_client_message(
            130,
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
        /* 0000 */ "\x02\x00\x00\x00\x7a\x00\x00\x00\x6e\xc0\x00\x00\x46\x00\x69\x00" // ....z...n...F.i.
        /* 0010 */ "\x6c\x00\x65\x00\x47\x00\x72\x00\x6f\x00\x75\x00\x70\x00\x44\x00" // l.e.G.r.o.u.p.D.
        /* 0020 */ "\x65\x00\x73\x00\x63\x00\x72\x00\x69\x00\x70\x00\x74\x00\x6f\x00" // e.s.c.r.i.p.t.o.
        /* 0030 */ "\x72\x00\x57\x00\x00\x00\x94\xc0\x00\x00\x46\x00\x69\x00\x6c\x00" // r.W.......F.i.l.
        /* 0040 */ "\x65\x00\x43\x00\x6f\x00\x6e\x00\x74\x00\x65\x00\x6e\x00\x74\x00" // e.C.o.n.t.e.n.t.
        /* 0050 */ "\x73\x00\x00\x00\x10\xc1\x00\x00\x50\x00\x72\x00\x65\x00\x66\x00" // s.......P.r.e.f.
        /* 0060 */ "\x65\x00\x72\x00\x72\x00\x65\x00\x64\x00\x20\x00\x44\x00\x72\x00" // e.r.r.e.d. .D.r.
        /* 0070 */ "\x6f\x00\x70\x00\x45\x00\x66\x00\x66\x00\x65\x00\x63\x00\x74\x00" // o.p.E.f.f.e.c.t.
        /* 0080 */ "\x00\x00"                                                         // ..
                ""_av);

// ClipboardVirtualChannel::process_server_format_data_request_pdu: requestedFormatId=<unknown>(49262)
// Sending on channel (16) n bytes
// /* 0000 */ "\x00\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x10\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x03\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x10\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x04\x00\x00\x00\x04\x00\x00\x00\x6e\xc0\x00\x00\x00\x00\x00\x00" // ........n.......
// Sent dumped on channel (16) n bytes
// CLIPRDR CB_FORMAT_DATA_RESPONSE format data id = 49262 <unknown>

    clipboard_virtual_channel.process_server_message(
            16,
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
        /* 0000 */ "\x04\x00\x00\x00\x04\x00\x00\x00\x6e\xc0\x00\x00\x00\x00\x00\x00"_av,
            out_asynchronous_task);

// ClipboardVirtualChannel::process_client_message: total_length=8 flags=0x00000013 chunk_data_length=8
// Recv done on channel (8) n bytes
// /* 0000 */ "\x00\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x08\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x13\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x08\x00\x00\x00"                                                 // ....
// /* 0000 */ "\x05\x00\x02\x00\x00\x00\x00\x00"                                 // ........
// Dump done on channel (8) n bytes
// ClipboardVirtualChannel::process_client_message: Format Data Response PDU

    clipboard_virtual_channel.process_client_message(
            8,
              CHANNELS::CHANNEL_FLAG_FIRST
            | CHANNELS::CHANNEL_FLAG_LAST
            | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL,
        /* 0000 */ "\x05\x00\x02\x00\x00\x00\x00\x00"_av);
}


class TestResponseSender : public VirtualChannelDataSender
{

public:
    StaticOutStream<1600> streams[10];
    size_t total_in_stream = 0;


    explicit TestResponseSender() = default;

    void operator()(uint32_t /*total_length*/, uint32_t /*flags*/, bytes_view chunk_data)
            override
    {
        if (this->total_in_stream < 10) {
            this->streams[this->total_in_stream].out_copy_bytes(chunk_data);
            this->total_in_stream++;
        }
    }

    bytes_view bytes(std::size_t i) const noexcept
    {
        return streams[i].get_bytes();
    }
};


struct Buffer
{
    StaticOutStream<1600> out;

    template<class F>
    bytes_view build(uint16_t msgType, uint16_t msgFlags, F f) &
    {
        using namespace RDPECLIP;
        array_view_u8 av = out.out_skip_bytes(CliprdrHeader::size());
        f(this->out);
        OutStream stream_header(av);
        CliprdrHeader(msgType, msgFlags, out.get_offset() - av.size()).emit(stream_header);
        return out.get_bytes();
    }
};

namespace
{
    using namespace std::string_view_literals;
    inline constexpr auto fdx_basename = "sid,blabla.fdx"sv;
}

RED_AUTO_TEST_CASE(TestCliprdrChannelFilterDataFile)
{
    struct ReportMessage : NullReportMessage
    {
        std::vector<std::string> messages;

        void log6(LogId id, const timeval /*time*/, KVList kv_list) override
        {
            std::string s = detail::log_id_string_map[int(id)].data();
            for (auto& kv : kv_list) {
                str_append(s, ' ', kv.key, '=', kv.value);
            }
            messages.emplace_back(std::move(s));
        }
    };

    struct D
    {
        bool with_validator;
        bool with_fdx_capture;
        bool always_file_record;
    };

    using namespace std::string_view_literals;

    static constexpr auto sid = "my_session_id"sv;

    RED_TEST_CONTEXT_DATA(D const& d,
          "with validator: " << d.with_validator <<
        "  with fdx: " << d.with_fdx_capture <<
        "  always_file_record: " << d.always_file_record, {
        D{true, false, true},
        D{true, true, false},
        D{true, true, true},
        D{false, false, true},
        D{false, true, false},
        D{false, true, true}
    })
    {
        struct DataTest
        {
            DataTest(char const* name)
            : wd(name)
            {}

            WorkingDirectory wd;
            WorkingDirectory::SubDirectory record_path = wd.create_subdirectory("record");
            WorkingDirectory::SubDirectory hash_path = wd.create_subdirectory("hash");
            WorkingDirectory::SubDirectory fdx_record_path = record_path.create_subdirectory(sid);
            WorkingDirectory::SubDirectory fdx_hash_path = hash_path.create_subdirectory(sid);
            CryptoContext cctx;
            LCGRandom rnd;
            FakeFstat fstat;
            FdxCapture fdx = FdxCapture{
                record_path.dirname().string(),
                hash_path.dirname().string(),
                fdx_basename, sid, -1, cctx, rnd, fstat,
                ReportError()};
        };

        auto fdx_ctx = [&]{
            if (d.with_fdx_capture) {
                return std::make_unique<DataTest>(
                    &"abcdefgh"[d.with_validator * 2 + d.always_file_record]
                );
            }
            return std::unique_ptr<DataTest>();
        }();

        {
            SessionReactor session_reactor;
            timeval time_test;
            time_test.tv_sec = 12345;
            time_test.tv_usec = 54321;
            session_reactor.set_current_time(time_test);

            ReportMessage report_message;
            BufTransport buf_trans;
            FileValidatorService file_validator_service(buf_trans);

            BaseVirtualChannel::Params base_params(report_message, RDPVerbose::cliprdr | RDPVerbose::cliprdr_dump);

            ClipboardVirtualChannelParams clipboard_virtual_channel_params;
            clipboard_virtual_channel_params.clipboard_down_authorized = true;
            clipboard_virtual_channel_params.clipboard_up_authorized   = true;
            clipboard_virtual_channel_params.clipboard_file_authorized = true;
            clipboard_virtual_channel_params.validator_params.down_target_name = "down";
            clipboard_virtual_channel_params.validator_params.up_target_name = "up";
            clipboard_virtual_channel_params.validator_params.log_if_accepted = true;

            TestResponseSender to_client_sender;
            TestResponseSender to_server_sender;

            ClipboardVirtualChannel clipboard_virtual_channel(
                &to_client_sender, &to_server_sender, session_reactor,
                base_params, clipboard_virtual_channel_params,
                d.with_validator ? &file_validator_service : nullptr,
                ClipboardVirtualChannel::FileRecord{
                    fdx_ctx ? &fdx_ctx->fdx : nullptr,
                    d.always_file_record
                }
            );

            std::unique_ptr<AsynchronousTask> out_asynchronous_task;

            auto process_server_message = [&](bytes_view av){
                auto flags
                = CHANNELS::CHANNEL_FLAG_FIRST
                | CHANNELS::CHANNEL_FLAG_LAST
                | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL;
                clipboard_virtual_channel.process_server_message(
                    av.size(), flags, av, out_asynchronous_task);
            };

            auto process_client_message = [&](bytes_view av){
                auto flags
                = CHANNELS::CHANNEL_FLAG_FIRST
                | CHANNELS::CHANNEL_FLAG_LAST
                | CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL;
                clipboard_virtual_channel.process_client_message(av.size(), flags, av);
            };

            const auto use_long_format = Cliprdr::IsLongFormat(true);
            const auto file_group = Cliprdr::formats::file_group_descriptor_w.ascii_name;
            const auto file_group_id = 49262;

            RED_CHECK(report_message.messages.size() == 0);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            {
                using namespace RDPECLIP;
                Buffer buf;
                auto av = buf.build(CB_CLIP_CAPS, 0, [&](OutStream& out){
                    out.out_uint16_le(CB_CAPSTYPE_GENERAL);
                    out.out_clear_bytes(2);
                    GeneralCapabilitySet{CB_CAPS_VERSION_1, CB_USE_LONG_FORMAT_NAMES}.emit(out);
                });

                process_client_message(av);
                process_server_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 1);
            RED_REQUIRE(to_server_sender.total_in_stream == 1);
            RED_CHECK_MEM(to_client_sender.bytes(0), to_server_sender.bytes(0));
            RED_CHECK_MEM(to_client_sender.bytes(0),
                "\x07\x00\x00\x00\x10\x00\x00\x00\x01\x00\x00\x00\x01\x00\x0c\x00" //................ !
                "\x01\x00\x00\x00\x02\x00\x00\x00" //........
                ""_av);
            RED_CHECK(report_message.messages.size() == 0);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            {
                StaticOutStream<1600> out;
                Cliprdr::format_list_serialize_with_header(
                    out,
                    Cliprdr::IsLongFormat(use_long_format),
                    std::array{Cliprdr::FormatNameRef{file_group_id, file_group}});

                process_client_message(out.get_bytes());
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 1);
            RED_REQUIRE(to_server_sender.total_in_stream == 2);
            RED_CHECK_MEM(to_server_sender.bytes(1),
                "\x02\x00\x00\x00\x2e\x00\x00\x00\x6e\xc0\x00\x00\x46\x00\x69\x00" //........n...F.i. !
                "\x6c\x00\x65\x00\x47\x00\x72\x00\x6f\x00\x75\x00\x70\x00\x44\x00" //l.e.G.r.o.u.p.D. !
                "\x65\x00\x73\x00\x63\x00\x72\x00\x69\x00\x70\x00\x74\x00\x6f\x00" //e.s.c.r.i.p.t.o. !
                "\x72\x00\x57\x00\x00\x00" //r.W... !
                ""_av);
            RED_CHECK(report_message.messages.size() == 0);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            // skip format list response

            {
                Buffer buf;
                auto av = buf.build(RDPECLIP::CB_FORMAT_DATA_REQUEST, 0, [&](OutStream& out){
                    out.out_uint32_le(file_group_id);
                });
                process_server_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 2);
            RED_REQUIRE(to_server_sender.total_in_stream == 2);
            RED_CHECK_MEM(to_client_sender.bytes(1),
                "\x04\x00\x00\x00\x04\x00\x00\x00\x6e\xc0\x00\x00"
                ""_av);
            RED_CHECK(report_message.messages.size() == 0);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            {
                using namespace Cliprdr;
                Buffer buf;
                auto av = buf.build(RDPECLIP::CB_FORMAT_DATA_RESPONSE, RDPECLIP::CB_RESPONSE_OK, [&](OutStream& out){
                    out.out_uint32_le(1);    // count item
                    out.out_uint32_le(0);    // flags
                    out.out_clear_bytes(32); // reserved1
                    out.out_uint32_le(0);    // attributes
                    out.out_clear_bytes(16); // reserved2
                    out.out_uint64_le(0);    // lastWriteTime
                    out.out_uint32_le(0);    // file size high
                    out.out_uint32_le(12);   // file size low
                    auto filename = "abc"_utf16_le;
                    out.out_copy_bytes(filename);
                    out.out_clear_bytes(520u - filename.size());
                });
                process_client_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 2);
            RED_REQUIRE(to_server_sender.total_in_stream == 3);
            RED_CHECK_MEM(to_server_sender.bytes(2),
                "\x05\x00\x01\x00\x54\x02\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" //....T........... !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x0c\x00\x00\x00\x61\x00\x62\x00\x63\x00\x00\x00\x00\x00\x00\x00" //....a.b.c....... !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //............ !
                ""_av);
            RED_REQUIRE(report_message.messages.size() == 1);
            RED_CHECK_SMEM(report_message.messages[0],
                "CB_COPYING_PASTING_DATA_TO_REMOTE_SESSION format=FileGroupDescriptorW(49262) size=596"_av);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            {
                using namespace RDPECLIP;
                Buffer buf;
                auto av = buf.build(CB_FILECONTENTS_REQUEST, CB_RESPONSE_OK, [&](OutStream& out){
                    FileContentsRequestPDU(0, 0, FILECONTENTS_RANGE, 0, 0, 12, 0, false).emit(out);
                    out.out_uint32_le(file_group_id);
                });
                process_server_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 3);
            RED_REQUIRE(to_server_sender.total_in_stream == 3);
            RED_CHECK_MEM(to_client_sender.bytes(2),
                "\x08\x00\x01\x00\x1c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //................ !
                "\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00" //................ !
                "\x6e\xc0\x00\x00" //n...
                ""_av);
            RED_CHECK(report_message.messages.size() == 1);

            if (d.with_validator) {
                RED_CHECK_SMEM(buf_trans.buf,
                    "\x07\x00\x00\x00\x19\x00\x00\x00\x01\x00\x02up\x00\x01\x00\b""filename\x00\x03""abc"
                    ""_av);
            }

            buf_trans.buf.clear();
            StaticOutStream<256> out;
            auto status = "ok"_av;

            using namespace LocalFileValidatorProtocol;
            FileValidatorHeader(MsgType::Result, 0/*len*/).emit(out);
            FileValidatorResultHeader{ValidationResult::IsAccepted, FileValidatorId(1),
                checked_int(status.size())}.emit(out);
            out.out_copy_bytes(status);
            auto av = out.get_bytes().as_chars();

            buf_trans.buf.assign(av.data(), av.size());
            clipboard_virtual_channel.DLP_antivirus_check_channels_files();
            buf_trans.buf.clear();

            size_t expected_report_messages_size = 1;

            if (d.with_validator) {
                ++expected_report_messages_size;
            }
            RED_REQUIRE(report_message.messages.size() == expected_report_messages_size);
            if (d.with_validator) {
                RED_CHECK_SMEM(report_message.messages[expected_report_messages_size-1u],
                    "FILE_VERIFICATION direction=UP file_name=abc status=ok"_av);
            }

            // file content

            {
                using namespace Cliprdr;
                Buffer buf;
                auto av = buf.build(RDPECLIP::CB_FILECONTENTS_RESPONSE, RDPECLIP::CB_RESPONSE_OK, [&](OutStream& out){
                    out.out_uint32_le(0);
                    out.out_copy_bytes("data_abcdefg"_av);
                });
                process_client_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 3);
            RED_REQUIRE(to_server_sender.total_in_stream == 4);
            RED_CHECK_MEM(to_server_sender.bytes(3),
                "\t\x00\x01\x00\x10\x00\x00\x00\x00\x00\x00\x00""data_abcdefg"
                ""_av);
            ++expected_report_messages_size;
            RED_REQUIRE(report_message.messages.size() == expected_report_messages_size);
            RED_CHECK_SMEM(report_message.messages[expected_report_messages_size-1u],
                "CB_COPYING_PASTING_FILE_TO_REMOTE_SESSION file_name=abc size=12"
                " sha256=d1b9c9db455c70b7c6a70225a00f859931e498f7f5e07f2c962e1078c0359f5e"_av);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            if (fdx_ctx && d.always_file_record) {
                auto basename = str_concat(sid, ",000001.tfl");
                auto tfl_path = fdx_ctx->fdx_record_path.add_file(basename);
                (void)fdx_ctx->fdx_hash_path.add_file(basename);
                std::string file_content = RED_CHECK_GET_FILE_CONTENTS(tfl_path);
                RED_CHECK(file_content == "data_abcdefg"sv);
            }

            // check TEXT_VALIDATION

            {
                StaticOutStream<1600> out;
                Cliprdr::format_list_serialize_with_header(
                    out,
                    Cliprdr::IsLongFormat(use_long_format),
                    std::array{Cliprdr::FormatNameRef{RDPECLIP::CF_TEXT, nullptr}});

                process_client_message(out.get_bytes());
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 3);
            RED_REQUIRE(to_server_sender.total_in_stream == 5);
            RED_CHECK_MEM(to_server_sender.bytes(4),
                "\x02\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x00\x00"
                ""_av);
            RED_CHECK(report_message.messages.size() == expected_report_messages_size);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            // skip format list response

            {
                Buffer buf;
                auto av = buf.build(RDPECLIP::CB_FORMAT_DATA_REQUEST, 0, [&](OutStream& out){
                    out.out_uint32_le(RDPECLIP::CF_UNICODETEXT);
                });
                process_server_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 4);
            RED_REQUIRE(to_server_sender.total_in_stream == 5);
            RED_CHECK_MEM(to_client_sender.bytes(3),
                "\x04\x00\x00\x00\x04\x00\x00\x00\r\x00\x00\x00"
                ""_av);
            RED_CHECK(report_message.messages.size() == expected_report_messages_size);
            RED_CHECK_SMEM(buf_trans.buf, ""_av);

            {
                using namespace Cliprdr;
                Buffer buf;
                auto av = buf.build(RDPECLIP::CB_FORMAT_DATA_RESPONSE, RDPECLIP::CB_RESPONSE_OK, [&](OutStream& out){
                    out.out_copy_bytes("a\0b\0c\0"_av);
                });
                process_client_message(av);
            }
            RED_REQUIRE(to_client_sender.total_in_stream == 4);
            RED_REQUIRE(to_server_sender.total_in_stream == 6);
            RED_CHECK_MEM(to_server_sender.bytes(5),
                "\x05\x00\x01\x00\x06\x00\x00\x00""a\x00""b\x00""c\x00"_av);
            ++expected_report_messages_size;
            RED_REQUIRE(report_message.messages.size() == expected_report_messages_size);
            RED_CHECK_SMEM(report_message.messages[expected_report_messages_size-1u],
            "CB_COPYING_PASTING_DATA_TO_REMOTE_SESSION_EX format=CF_UNICODETEXT(13) size=6 partial_data=abc"_av);
            if (d.with_validator) {
                RED_CHECK_SMEM(buf_trans.buf,
                    "\x07\x00\x00\x00\"\x00\x00\x00\x02\x00\x02up\x00\x01\x00\x13microsoft_locale_id\x00\x01"
                    "0\x01\x00\x00\x00\x07\x00\x00\x00\x02""abc\x03\x00\x00\x00\x04\x00\x00\x00\x02"_av);
                buf_trans.buf.clear();
            }

            out.rewind();
            FileValidatorHeader(MsgType::Result, 0/*len*/).emit(out);
            FileValidatorResultHeader{ValidationResult::IsAccepted, FileValidatorId(2),
                checked_int(status.size())}.emit(out);
            out.out_copy_bytes(status);
            av = out.get_bytes().as_chars();
            buf_trans.buf.assign(av.data(), av.size());

            clipboard_virtual_channel.DLP_antivirus_check_channels_files();
            if (d.with_validator) {
                ++expected_report_messages_size;
            }
            RED_REQUIRE(report_message.messages.size() == expected_report_messages_size);
            if (d.with_validator) {
                RED_CHECK_SMEM(report_message.messages[expected_report_messages_size-1u],
                    "TEXT_VERIFICATION direction=UP copy_id=2 status=ok"_av);
            }
        }

        if (fdx_ctx) {
            (void)fdx_ctx->hash_path.add_file(fdx_basename);
            auto fdx_path = fdx_ctx->record_path.add_file(fdx_basename);

            OutCryptoTransport::HashArray qhash;
            OutCryptoTransport::HashArray fhash;
            fdx_ctx->fdx.close(qhash, fhash);

            RED_CHECK_WORKSPACE(fdx_ctx->wd);

            if (d.always_file_record) {
                std::string file_content = RED_CHECK_GET_FILE_CONTENTS(fdx_path);
                bytes_view av = file_content;

                RED_CHECK(av ==
                    "v3\n\x04\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00,\x03\x00\x18\x00""abcmy_session_id,000001.tfl\xd1\xb9\xc9"
                    "\xdb""E\\p\xb7\xc6\xa7\x02%\xa0\x0f\x85\x99""1\xe4\x98\xf7\xf5\xe0"
                    "\x7f,\x96.\x10x\xc0""5\x9f^"_av);
            }
        }
    }
}

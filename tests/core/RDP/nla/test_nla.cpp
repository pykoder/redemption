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
  Copyright (C) Wallix 2013
  Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "core/RDP/nla/nla_server_ntlm.hpp"
#include "core/RDP/nla/nla_client_ntlm.hpp"
#include "core/RDP/tpdu_buffer.hpp"

#include "test_only/transport/test_transport.hpp"

#include "test_only/lcg_random.hpp"


RED_TEST_DELEGATE_PRINT_ENUM(credssp::State);

RED_AUTO_TEST_CASE(TestNlaclient)
{
    auto public_key = std::vector<uint8_t>{} << bytes_view("1245789652325415"_av); 
    std::string user("Ulysse");
    std::string domain("Ithaque");
    uint8_t pass[] = "Pénélope";
    uint8_t host[] = "Télémaque";
    LCGRandom rand(0);
    LCGTime timeobj;
    std::string extra_message;
    rdpClientNTLM ntlm_client(user, domain, pass, host, "107.0.0.1", public_key, false, rand, timeobj);

    std::vector<uint8_t> expected_negotiate{
/* 0000 */ 0x30, 0x37, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa1, 0x30, 0x30, 0x2e, 0x30, 0x2c, 0xa0, 0x2a, 0x04,  // 07......00.0,.*.
/* 0010 */ 0x28, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08,  // (NTLMSSP........
/* 0020 */ 0xe2, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00,  // .....(.......(..
/* 0030 */ 0x00, 0x06, 0x01, 0xb1, 0x1d, 0x00, 0x00, 0x00, 0x0f,  
    };

    auto negotiate_message = ntlm_client.authenticate_start();
    RED_CHECK_HMEM(negotiate_message, expected_negotiate);

    std::vector<uint8_t> server_answer_challenge{
    /* 0000 */ 0x30, 0x81, 0x88, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa1, 0x81, 0x80, 0x30, 0x7e, 0x30, 0x7c, 0xa0,  // 0..........0~0|.
    /* 0010 */ 0x7a, 0x04, 0x78, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,  // z.xNTLMSSP......
    /* 0020 */ 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08, 0xe2, 0xb8, 0x6c, 0xda, 0xa6, 0xf0,  // ...8........l...
    /* 0030 */ 0xf6, 0x30, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x38,  // .0.........@.@.8
    /* 0040 */ 0x00, 0x00, 0x00, 0x05, 0x01, 0x28, 0x0a, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x08, 0x00, 0x57,  // .....(.........W
    /* 0050 */ 0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e,  // .I.N.7.....W.I.N
    /* 0060 */ 0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x04,  // .7.....w.i.n.7..
    /* 0070 */ 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0x67,  // ...w.i.n.7.....g
    /* 0080 */ 0x95, 0x0e, 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0x00, 0x00, 0x00, 0x00,                                // ..ZNVv.....
    };

    std::vector<uint8_t> expected_authenticate{
    /* 0000 */ 0x30, 0x82, 0x01, 0x59, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa1, 0x82, 0x01, 0x2c, 0x30, 0x82, 0x01,  // 0..Y........,0..
    /* 0010 */ 0x28, 0x30, 0x82, 0x01, 0x24, 0xa0, 0x82, 0x01, 0x20, 0x04, 0x82, 0x01, 0x1c, 0x4e, 0x54, 0x4c,  // (0..$... ....NTL
    /* 0020 */ 0x4d, 0x53, 0x53, 0x50, 0x00, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x18, 0x00, 0x58, 0x00, 0x00,  // MSSP.........X..
    /* 0030 */ 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0xe0, 0x00, 0x00,  // .p.p.p..........
    /* 0040 */ 0x00, 0x0c, 0x00, 0x0c, 0x00, 0xee, 0x00, 0x00, 0x00, 0x12, 0x00, 0x12, 0x00, 0xfa, 0x00, 0x00,  // ................
    /* 0050 */ 0x00, 0x10, 0x00, 0x10, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x35, 0xa2, 0x88, 0xe2, 0x06, 0x01, 0xb1,  // .........5......
    /* 0060 */ 0x1d, 0x00, 0x00, 0x00, 0x0f, 0x9d, 0x98, 0xe1, 0x30, 0xdc, 0x7a, 0xce, 0x81, 0xa9, 0x7a, 0x11,  // ........0.z...z.
    /* 0070 */ 0x14, 0x7d, 0x5d, 0x73, 0xd0, 0x34, 0xbe, 0xb5, 0x6e, 0xce, 0xec, 0x0a, 0x50, 0x2d, 0x29, 0x63,  // .}]s.4..n...P-)c
    /* 0080 */ 0x7f, 0xcc, 0x5e, 0xe7, 0x18, 0xb8, 0x6c, 0xda, 0xa6, 0xf0, 0xf6, 0x30, 0x8d, 0xa6, 0x41, 0xcc,  // ..^...l....0..A.
    /* 0090 */ 0x7a, 0x52, 0x8e, 0x7a, 0xb3, 0x06, 0x7d, 0x0b, 0xe0, 0x00, 0xd5, 0xf6, 0x13, 0x01, 0x01, 0x00,  // zR.z..}.........
    /* 00a0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x95, 0x0e, 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0xb8, 0x6c, 0xda,  // .....g..ZNVv..l.
    /* 00b0 */ 0xa6, 0xf0, 0xf6, 0x30, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49,  // ...0.........W.I
    /* 00c0 */ 0x00, 0x4e, 0x00, 0x37, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e, 0x00, 0x37,  // .N.7.....W.I.N.7
    /* 00d0 */ 0x00, 0x03, 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x04, 0x00, 0x08,  // .....w.i.n.7....
    /* 00e0 */ 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0x67, 0x95, 0x0e,  // .w.i.n.7.....g..
    /* 00f0 */ 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x00, 0x74,  // ZNVv.........I.t
    /* 0100 */ 0x00, 0x68, 0x00, 0x61, 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x55, 0x00, 0x6c, 0x00, 0x79,  // .h.a.q.u.e.U.l.y
    /* 0110 */ 0x00, 0x73, 0x00, 0x73, 0x00, 0x65, 0x00, 0x54, 0x00, 0xe9, 0x00, 0x6c, 0x00, 0xe9, 0x00, 0x6d,  // .s.s.e.T...l...m
    /* 0120 */ 0x00, 0x61, 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x44, 0xbc, 0x4d, 0x7a, 0x13, 0x3f, 0x6b,  // .a.q.u.e.D.Mz.?k
    /* 0130 */ 0x81, 0xdb, 0x1d, 0x2b, 0x7b, 0xbf, 0x1e, 0x18, 0x0f, 0xa3, 0x22, 0x04, 0x20, 0x01, 0x00, 0x00,  // ...+{.....". ...
    /* 0140 */ 0x00, 0x89, 0xe2, 0xda, 0x48, 0x17, 0x29, 0xb5, 0x08, 0x00, 0x00, 0x00, 0x00, 0x20, 0x59, 0x27,  // ....H.)...... Y'
    /* 0150 */ 0x3f, 0x08, 0xd0, 0xc2, 0xe4, 0x75, 0x66, 0x10, 0x49, 0x7b, 0xbd, 0x8d, 0xf7,                    // ?....uf.I{...
    };
    StaticOutStream<65536> buffer_to_send_authenticate;
    credssp::State st1 = ntlm_client.authenticate_next(server_answer_challenge, buffer_to_send_authenticate);
    RED_CHECK(credssp::State::Cont == st1);
    RED_CHECK_HMEM(buffer_to_send_authenticate.get_bytes(), expected_authenticate);

    std::vector<uint8_t> server_answer_pubauthkey{
        /* 0000 */ 0x30, 0x29, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa3, 0x22, 0x04, 0x20, 0x01, 0x00, 0x00, 0x00, 0xa2,  // 0)......". .....
        /* 0010 */ 0xe0, 0x5b, 0x50, 0x97, 0x8e, 0x99, 0x27, 0x00, 0x00, 0x00, 0x00, 0xdc, 0xa7, 0x0b, 0xfe, 0x37,  // .[P...'........7
        /* 0020 */ 0x45, 0x3d, 0x1b, 0x05, 0x15, 0xce, 0x56, 0x0a, 0x54, 0xa1, 0xf1,                                // E=....V.T..
    };
    
    std::vector<uint8_t> expected_tscredentials{
        /* 0000 */ 0x30, 0x5c, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa2, 0x55, 0x04, 0x53, 0x01, 0x00, 0x00, 0x00, 0xaf,  // 0.......U.S.....
        /* 0010 */ 0xad, 0x46, 0x2a, 0x6a, 0x9d, 0xf7, 0x88, 0x01, 0x00, 0x00, 0x00, 0xd5, 0x4f, 0xc8, 0xd0, 0xbd,  // .F*j........O...
        /* 0020 */ 0x89, 0x60, 0xe0, 0x71, 0x60, 0x31, 0x7a, 0xcc, 0xec, 0xc5, 0xbf, 0x23, 0x4b, 0xe5, 0xf9, 0xa5,  // .`.q`1z....#K...
        /* 0030 */ 0x8c, 0x21, 0x66, 0xa6, 0x78, 0xda, 0xd1, 0xbd, 0xef, 0xa4, 0xfd, 0x47, 0xa6, 0xf1, 0x56, 0xa5,  // .!f.x......G..V.
        /* 0040 */ 0xd9, 0x52, 0x72, 0x92, 0xfa, 0x41, 0xa5, 0xb4, 0x9d, 0x94, 0xfb, 0x0e, 0xe2, 0x61, 0xba, 0xfc,  // .Rr..A.......a..
        /* 0050 */ 0xd5, 0xf3, 0xa7, 0xb5, 0x33, 0xd5, 0x62, 0x8d, 0x93, 0x18, 0x54, 0x39, 0x8a, 0xe7,              // ....3.b...T9..
    };
    
    StaticOutStream<65536> buffer_to_send_tscredentials;
    credssp::State st2 = ntlm_client.authenticate_next(server_answer_pubauthkey, buffer_to_send_tscredentials);
    RED_CHECK(credssp::State::Finish == st2);
    RED_CHECK_HMEM(buffer_to_send_tscredentials.get_bytes(), expected_tscredentials);
}



RED_AUTO_TEST_CASE(TestNlaserver)
{
    auto user = "Ulysse"_av;
    auto domain = "Ithaque"_av;
    auto pass = "Pénélope"_av;
//    auto host = "Télémaque"_av;
    LCGRandom rand(0);
    LCGTime timeobj;
    std::string extra_message;
    Translation::language_t lang = Translation::EN;

    auto get_password = [&](bytes_view user_av, bytes_view domain_av, std::vector<uint8_t> & password_array){
            std::vector<uint8_t> vec;
            vec.resize(user.size() * 2);
            UTF8toUTF16(user, vec.data(), vec.size());
            RED_CHECK_MEM(user_av, vec);
            vec.resize(domain.size() * 2);
            UTF8toUTF16(domain, vec.data(), vec.size());
            RED_CHECK_MEM(domain_av, vec);
            size_t user_len = UTF8Len(byte_ptr_cast(pass.data()));
            LOG(LOG_INFO, "callback lambda: user_len=%lu", user_len);
            password_array = std::vector<uint8_t>(user_len * 2);
            UTF8toUTF16({pass.data(), strlen(char_ptr_cast(byte_ptr_cast(pass.data())))}, password_array.data(), user_len * 2);
            return PasswordCallback::Ok;
        }; 

    auto public_key = std::vector<uint8_t>{} << bytes_view("1245789652325415"_av); 
    NtlmServer ntlm_server(public_key, rand, timeobj, extra_message, lang, get_password, true);
    credssp::State st = credssp::State::Cont;

    std::vector<uint8_t> negotiate{ 
       0x30, 0x37, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa1, 0x30, 0x30, 0x2e, 0x30, 0x2c, 0xa0, 0x2a, 0x04, //07......00.0,.*. !
       0x28, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08, //(NTLMSSP........ !
       0xe2, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, //.....(.......(.. !
       0x00, 0x06, 0x01, 0xb1, 0x1d, 0x00, 0x00, 0x00, 0x0f                                            //......... !
    };
    // Recv negotiate, send challenge
    {
        StaticOutStream<65536> out_stream;
        LOG(LOG_INFO, "Recv Negotiate");
        st = ntlm_server.authenticate_next(negotiate, out_stream);

        std::vector<uint8_t> challenge{
            0x30, 0x81, 0x88, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa1, 0x81, 0x80, 0x30, 0x7e, 0x30, 0x7c, 0xa0, //0..........0~0|. !
            0x7a, 0x04, 0x78, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, //z.xNTLMSSP...... !
            0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08, 0xe2, 0xb8, 0x6c, 0xda, 0xa6, 0xf0, //...8........l... !
            0xf6, 0x30, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x38, //.0.........@.@.8 !
            0x00, 0x00, 0x00, 0x06, 0x01, 0xb1, 0x1d, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x08, 0x00, 0x57, //...............W !
            0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e, //.I.N.7.....W.I.N !
            0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x04, //.7.....w.i.n.7.. !
            0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0x67, //...w.i.n.7.....g !
            0x95, 0x0e, 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0x00, 0x00, 0x00, 0x00                                //..ZNVv..... !]
        };
        RED_CHECK_HMEM(challenge, out_stream.get_bytes());
    }
    
    RED_CHECK_EQUAL(credssp::State::Cont, st);

    // authenticate + pubauthkey
    std::vector<uint8_t> authenticate{ 
        0x30, 0x82, 0x01, 0x59, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa1, 0x82, 0x01, 0x2c, 0x30, 0x82, 0x01, //0..Y........,0.. !
        0x28, 0x30, 0x82, 0x01, 0x24, 0xa0, 0x82, 0x01, 0x20, 0x04, 0x82, 0x01, 0x1c, 0x4e, 0x54, 0x4c, //(0..$... ....NTL !
        0x4d, 0x53, 0x53, 0x50, 0x00, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x18, 0x00, 0x58, 0x00, 0x00, //MSSP.........X.. !
        0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0xe0, 0x00, 0x00, //.p.p.p.......... !
        0x00, 0x0c, 0x00, 0x0c, 0x00, 0xee, 0x00, 0x00, 0x00, 0x12, 0x00, 0x12, 0x00, 0xfa, 0x00, 0x00, //................ !
        0x00, 0x10, 0x00, 0x10, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x35, 0xa2, 0x88, 0xe2, 0x06, 0x01, 0xb1, //.........5...... !
        0x1d, 0x00, 0x00, 0x00, 0x0f, 0x2e, 0x5b, 0xe2, 0x1f, 0x57, 0x20, 0x79, 0xa8, 0x5c, 0x70, 0x2d, //......[..W y..p- !
        0x3d, 0xb6, 0x46, 0x81, 0x9a, 0x34, 0xbe, 0xb5, 0x6e, 0xce, 0xec, 0x0a, 0x50, 0x2d, 0x29, 0x63, //=.F..4..n...P-)c !
        0x7f, 0xcc, 0x5e, 0xe7, 0x18, 0xb8, 0x6c, 0xda, 0xa6, 0xf0, 0xf6, 0x30, 0x8d, 0xa6, 0x41, 0xcc, //.^...l....0..A. !
        0x7a, 0x52, 0x8e, 0x7a, 0xb3, 0x06, 0x7d, 0x0b, 0xe0, 0x00, 0xd5, 0xf6, 0x13, 0x01, 0x01, 0x00, //zR.z..}......... !
        0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x95, 0x0e, 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0xb8, 0x6c, 0xda, //.....g..ZNVv..l. !
        0xa6, 0xf0, 0xf6, 0x30, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, //...0.........W.I !
        0x00, 0x4e, 0x00, 0x37, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, //.N.7.....W.I.N.7 !
        0x00, 0x03, 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x04, 0x00, 0x08, //.....w.i.n.7.... !
        0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0x67, 0x95, 0x0e, //.w.i.n.7.....g.. !
        0x5a, 0x4e, 0x56, 0x76, 0xd6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x00, 0x74, //ZNVv.........I.t !
        0x00, 0x68, 0x00, 0x61, 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x55, 0x00, 0x6c, 0x00, 0x79, //.h.a.q.u.e.U.l.y !
        0x00, 0x73, 0x00, 0x73, 0x00, 0x65, 0x00, 0x54, 0x00, 0xe9, 0x00, 0x6c, 0x00, 0xe9, 0x00, 0x6d, //.s.s.e.T...l...m !
        0x00, 0x61, 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x44, 0xbc, 0x4d, 0x7a, 0x13, 0x3f, 0x6b, //.a.q.u.e.D.Mz.?k !
        0x81, 0xdb, 0x1d, 0x2b, 0x7b, 0xbf, 0x1e, 0x18, 0x0f, 0xa3, 0x22, 0x04, 0x20, 0x01, 0x00, 0x00, //...+{.....". ... !
        0x00, 0x89, 0xe2, 0xda, 0x48, 0x17, 0x29, 0xb5, 0x08, 0x00, 0x00, 0x00, 0x00, 0x20, 0x59, 0x27, //....H.)...... Y' !
        0x3f, 0x08, 0xd0, 0xc2, 0xe4, 0x75, 0x66, 0x10, 0x49, 0x7b, 0xbd, 0x8d, 0xf7                    //?....uf.I{... !]
    };

    // Recv authenticate + pubauthkey, send pubauthkey
    {
        StaticOutStream<65536> out_stream;
        LOG(LOG_INFO, "Recv authenticate");
        
        st = ntlm_server.authenticate_next(authenticate, out_stream);

        std::vector<uint8_t> pubauthkey{
            0x30, 0x29, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa3, 0x22, 0x04, 0x20, 0x01, 0x00, 0x00, 0x00, 0xa2, //0)......". .....
            0xe0, 0x5b, 0x50, 0x97, 0x8e, 0x99, 0x27, 0x00, 0x00, 0x00, 0x00, 0xdc, 0xa7, 0x0b, 0xfe, 0x37, //.[P...'........7
            0x45, 0x3d, 0x1b, 0x05, 0x15, 0xce, 0x56, 0x0a, 0x54, 0xa1, 0xf1,                     //E=....V.T..
        };
        RED_CHECK_HMEM(pubauthkey, out_stream.get_bytes());
    }
    
    RED_CHECK_EQUAL(credssp::State::Cont, st);
    
    std::vector<uint8_t>  ts_credentials{
        0x30, 0x5c, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa2, 0x55, 0x04, 0x53, 0x01, 0x00, 0x00, 0x00, 0xaf, // 0.......U.S.....
        0xad, 0x46, 0x2a, 0x6a, 0x9d, 0xf7, 0x88, 0x01, 0x00, 0x00, 0x00, 0xd5, 0x4f, 0xc8, 0xd0, 0xbd, // .F*j........O...
        0x89, 0x60, 0xe0, 0x71, 0x60, 0x31, 0x7a, 0xcc, 0xec, 0xc5, 0xbf, 0x23, 0x4b, 0xe5, 0xf9, 0xa5, // .`.q`1z....#K...
        0x8c, 0x21, 0x66, 0xa6, 0x78, 0xda, 0xd1, 0xbd, 0xef, 0xa4, 0xfd, 0x47, 0xa6, 0xf1, 0x56, 0xa5, // .!f.x......G..V.
        0xd9, 0x52, 0x72, 0x92, 0xfa, 0x41, 0xa5, 0xb4, 0x9d, 0x94, 0xfb, 0x0e, 0xe2, 0x61, 0xba, 0xfc, // .Rr..A.......a..
        0xd5, 0xf3, 0xa7, 0xb5, 0x33, 0xd5, 0x62, 0x8d, 0x93, 0x18, 0x54, 0x39, 0x8a, 0xe7              // ....3.b...T9..
    };
    
    // Recv ts_credential, -> finished
    {
        StaticOutStream<65536> out_stream;
        st = ntlm_server.authenticate_next(ts_credentials, out_stream);
        RED_CHECK_EQUAL(out_stream.get_bytes().size(), 0);
    }
    RED_CHECK_EQUAL(st, credssp::State::Finish);
}


// Tpdu class is used to extract one Credssp packet from stream in authentication sequence
// and afterward to commute to extraction of an RDP TPKT (PDU)
RED_AUTO_TEST_CASE(TestTpdu)
{
    RED_TEST_PASSPOINT();
    auto client =
        // negotiate
    "\x30\x37\xa0\x03\x02\x01\x06\xa1\x30\x30\x2e\x30\x2c\xa0\x2a\x04" //07......00.0,.*. !
    "\x28\x4e\x54\x4c\x4d\x53\x53\x50\x00\x01\x00\x00\x00\xb7\x82\x08" //(NTLMSSP........ !
    "\xe2\x00\x00\x00\x00\x28\x00\x00\x00\x00\x00\x00\x00\x28\x00\x00" //.....(.......(.. !
    "\x00\x06\x01\xb1\x1d\x00\x00\x00\x0f" //......... !
        // authenticate + pubauthkey
    "\x30\x82\x01\x59\xa0\x03\x02\x01\x02\xa1\x82\x01\x2c\x30\x82\x01" //0..Y........,0.. !
    "\x28\x30\x82\x01\x24\xa0\x82\x01\x20\x04\x82\x01\x1c\x4e\x54\x4c" //(0..$... ....NTL !
    "\x4d\x53\x53\x50\x00\x03\x00\x00\x00\x18\x00\x18\x00\x58\x00\x00" //MSSP.........X.. !
    "\x00\x70\x00\x70\x00\x70\x00\x00\x00\x0e\x00\x0e\x00\xe0\x00\x00" //.p.p.p.......... !
    "\x00\x0c\x00\x0c\x00\xee\x00\x00\x00\x12\x00\x12\x00\xfa\x00\x00" //................ !
    "\x00\x10\x00\x10\x00\x0c\x01\x00\x00\x35\xa2\x88\xe2\x06\x01\xb1" //.........5...... !
    "\x1d\x00\x00\x00\x0f\x2e\x5b\xe2\x1f\x57\x20\x79\xa8\x5c\x70\x2d" //......[..W y..p- !
    "\x3d\xb6\x46\x81\x9a\x34\xbe\xb5\x6e\xce\xec\x0a\x50\x2d\x29\x63" //=.F..4..n...P-)c !
    "\x7f\xcc\x5e\xe7\x18\xb8\x6c\xda\xa6\xf0\xf6\x30\x8d\xa6\x41\xcc" //.^...l....0..A. !
    "\x7a\x52\x8e\x7a\xb3\x06\x7d\x0b\xe0\x00\xd5\xf6\x13\x01\x01\x00" //zR.z..}......... !
    "\x00\x00\x00\x00\x00\x67\x95\x0e\x5a\x4e\x56\x76\xd6\xb8\x6c\xda" //.....g..ZNVv..l. !
    "\xa6\xf0\xf6\x30\x8d\x00\x00\x00\x00\x01\x00\x08\x00\x57\x00\x49" //...0.........W.I !
    "\x00\x4e\x00\x37\x00\x02\x00\x08\x00\x57\x00\x49\x00\x4e\x00\x37" //.N.7.....W.I.N.7 !
    "\x00\x03\x00\x08\x00\x77\x00\x69\x00\x6e\x00\x37\x00\x04\x00\x08" //.....w.i.n.7.... !
    "\x00\x77\x00\x69\x00\x6e\x00\x37\x00\x07\x00\x08\x00\x67\x95\x0e" //.w.i.n.7.....g.. !
    "\x5a\x4e\x56\x76\xd6\x00\x00\x00\x00\x00\x00\x00\x00\x49\x00\x74" //ZNVv.........I.t !
    "\x00\x68\x00\x61\x00\x71\x00\x75\x00\x65\x00\x55\x00\x6c\x00\x79" //.h.a.q.u.e.U.l.y !
    "\x00\x73\x00\x73\x00\x65\x00\x54\x00\xe9\x00\x6c\x00\xe9\x00\x6d" //.s.s.e.T...l...m !
    "\x00\x61\x00\x71\x00\x75\x00\x65\x00\x44\xbc\x4d\x7a\x13\x3f\x6b" //.a.q.u.e.D.Mz.?k !
    "\x81\xdb\x1d\x2b\x7b\xbf\x1e\x18\x0f\xa3\x22\x04\x20\x01\x00\x00" //...+{.....". ... !
    "\x00\x89\xe2\xda\x48\x17\x29\xb5\x08\x00\x00\x00\x00\x20\x59\x27" //....H.)...... Y' !
    "\x3f\x08\xd0\xc2\xe4\x75\x66\x10\x49\x7b\xbd\x8d\xf7" //?....uf.I{... !]

        // ts credentials (authinfo)
/* 0000 */ "\x30\x5c\xa0\x03\x02\x01\x02\xa2\x55\x04\x53\x01\x00\x00\x00\xaf" // 0.......U.S.....
/* 0010 */ "\xad\x46\x2a\x6a\x9d\xf7\x88\x01\x00\x00\x00\xd5\x4f\xc8\xd0\xbd" // .F*j........O...
/* 0020 */ "\x89\x60\xe0\x71\x60\x31\x7a\xcc\xec\xc5\xbf\x23\x4b\xe5\xf9\xa5" // .`.q`1z....#K...
/* 0030 */ "\x8c\x21\x66\xa6\x78\xda\xd1\xbd\xef\xa4\xfd\x47\xa6\xf1\x56\xa5" // .!f.x......G..V.
/* 0040 */ "\xd9\x52\x72\x92\xfa\x41\xa5\xb4\x9d\x94\xfb\x0e\xe2\x61\xba\xfc" // .Rr..A.......a..
/* 0050 */ "\xd5\xf3\xa7\xb5\x33\xd5\x62\x8d\x93\x18\x54\x39\x8a\xe7"         // ....3.b...T9..
    ""_av
        ;

    auto server =
        // challenge
    "\x30\x81\x88\xa0\x03\x02\x01\x06\xa1\x81\x80\x30\x7e\x30\x7c\xa0" //0..........0~0|. !
    "\x7a\x04\x78\x4e\x54\x4c\x4d\x53\x53\x50\x00\x02\x00\x00\x00\x00" //z.xNTLMSSP...... !
    "\x00\x00\x00\x38\x00\x00\x00\xb7\x82\x08\xe2\xb8\x6c\xda\xa6\xf0" //...8........l... !
    "\xf6\x30\x8d\x00\x00\x00\x00\x00\x00\x00\x00\x40\x00\x40\x00\x38" //.0.........@.@.8 !
    "\x00\x00\x00\x06\x01\xb1\x1d\x00\x00\x00\x0f\x01\x00\x08\x00\x57" //...............W !
    "\x00\x49\x00\x4e\x00\x37\x00\x02\x00\x08\x00\x57\x00\x49\x00\x4e" //.I.N.7.....W.I.N !
    "\x00\x37\x00\x03\x00\x08\x00\x77\x00\x69\x00\x6e\x00\x37\x00\x04" //.7.....w.i.n.7.. !
    "\x00\x08\x00\x77\x00\x69\x00\x6e\x00\x37\x00\x07\x00\x08\x00\x67" //...w.i.n.7.....g !
    "\x95\x0e\x5a\x4e\x56\x76\xd6\x00\x00\x00\x00" //..ZNVv..... !]
//         // pubauthkey
/* 0000 */ "\x30\x29\xa0\x03\x02\x01\x06\xa3\x22\x04\x20\x01\x00\x00\x00\xa2" //0)......". .....
/* 0010 */ "\xe0\x5b\x50\x97\x8e\x99\x27\x00\x00\x00\x00\xdc\xa7\x0b\xfe\x37" //.[P...'........7
/* 0020 */ "\x45\x3d\x1b\x05\x15\xce\x56\x0a\x54\xa1\xf1"                     //E=....V.T..
    ""_av
        ;

    TestTransport logtrans(client, server);
    TpduBuffer buf;
    buf.load_data(logtrans);
    RED_CHECK_EQUAL(buf.remaining(), 500);
    RED_CHECK_EQUAL(true, buf.next(TpduBuffer::CREDSSP));

    std::vector<uint8_t> negotiate{ 
       0x30, 0x37, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa1, 0x30, 0x30, 0x2e, 0x30, 0x2c, 0xa0, 0x2a, 0x04, //07......00.0,.*. !
       0x28, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08, //(NTLMSSP........ !
       0xe2, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, //.....(.......(.. !
       0x00, 0x06, 0x01, 0xb1, 0x1d, 0x00, 0x00, 0x00, 0x0f                                            //......... !
    };

    RED_CHECK_EQUAL(buf.remaining(), 500);
    RED_CHECK_HMEM(negotiate, buf.current_pdu_buffer());

    std::vector<uint8_t> challenge{
        0x30, 0x81, 0x88, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa1, 0x81, 0x80, 0x30, 0x7e, 0x30, 0x7c, 0xa0, //0..........0~0|. !
        0x7a, 0x04, 0x78, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, //z.xNTLMSSP...... !
        0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08, 0xe2, 0xb8, 0x6c, 0xda, 0xa6, 0xf0, //...8........l... !
        0xf6, 0x30, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x38, //.0.........@.@.8 !
        0x00, 0x00, 0x00, 0x06, 0x01, 0xb1, 0x1d, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x08, 0x00, 0x57, //...............W !
        0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e, //.I.N.7.....W.I.N !
        0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x04, //.7.....w.i.n.7.. !
        0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0x67, //...w.i.n.7.....g !
        0x95, 0x0e, 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0x00, 0x00, 0x00, 0x00                                //..ZNVv..... !]
    };

    logtrans.send(challenge);

    std::vector<uint8_t> authenticate{ 
        0x30, 0x82, 0x01, 0x59, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa1, 0x82, 0x01, 0x2c, 0x30, 0x82, 0x01, //0..Y........,0.. !
        0x28, 0x30, 0x82, 0x01, 0x24, 0xa0, 0x82, 0x01, 0x20, 0x04, 0x82, 0x01, 0x1c, 0x4e, 0x54, 0x4c, //(0..$... ....NTL !
        0x4d, 0x53, 0x53, 0x50, 0x00, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x18, 0x00, 0x58, 0x00, 0x00, //MSSP.........X.. !
        0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0xe0, 0x00, 0x00, //.p.p.p.......... !
        0x00, 0x0c, 0x00, 0x0c, 0x00, 0xee, 0x00, 0x00, 0x00, 0x12, 0x00, 0x12, 0x00, 0xfa, 0x00, 0x00, //................ !
        0x00, 0x10, 0x00, 0x10, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x35, 0xa2, 0x88, 0xe2, 0x06, 0x01, 0xb1, //.........5...... !
        0x1d, 0x00, 0x00, 0x00, 0x0f, 0x2e, 0x5b, 0xe2, 0x1f, 0x57, 0x20, 0x79, 0xa8, 0x5c, 0x70, 0x2d, //......[..W y..p- !
        0x3d, 0xb6, 0x46, 0x81, 0x9a, 0x34, 0xbe, 0xb5, 0x6e, 0xce, 0xec, 0x0a, 0x50, 0x2d, 0x29, 0x63, //=.F..4..n...P-)c !
        0x7f, 0xcc, 0x5e, 0xe7, 0x18, 0xb8, 0x6c, 0xda, 0xa6, 0xf0, 0xf6, 0x30, 0x8d, 0xa6, 0x41, 0xcc, //.^...l....0..A. !
        0x7a, 0x52, 0x8e, 0x7a, 0xb3, 0x06, 0x7d, 0x0b, 0xe0, 0x00, 0xd5, 0xf6, 0x13, 0x01, 0x01, 0x00, //zR.z..}......... !
        0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x95, 0x0e, 0x5a, 0x4e, 0x56, 0x76, 0xd6, 0xb8, 0x6c, 0xda, //.....g..ZNVv..l. !
        0xa6, 0xf0, 0xf6, 0x30, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, //...0.........W.I !
        0x00, 0x4e, 0x00, 0x37, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, //.N.7.....W.I.N.7 !
        0x00, 0x03, 0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x04, 0x00, 0x08, //.....w.i.n.7.... !
        0x00, 0x77, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0x67, 0x95, 0x0e, //.w.i.n.7.....g.. !
        0x5a, 0x4e, 0x56, 0x76, 0xd6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x00, 0x74, //ZNVv.........I.t !
        0x00, 0x68, 0x00, 0x61, 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x55, 0x00, 0x6c, 0x00, 0x79, //.h.a.q.u.e.U.l.y !
        0x00, 0x73, 0x00, 0x73, 0x00, 0x65, 0x00, 0x54, 0x00, 0xe9, 0x00, 0x6c, 0x00, 0xe9, 0x00, 0x6d, //.s.s.e.T...l...m !
        0x00, 0x61, 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x44, 0xbc, 0x4d, 0x7a, 0x13, 0x3f, 0x6b, //.a.q.u.e.D.Mz.?k !
        0x81, 0xdb, 0x1d, 0x2b, 0x7b, 0xbf, 0x1e, 0x18, 0x0f, 0xa3, 0x22, 0x04, 0x20, 0x01, 0x00, 0x00, //...+{.....". ... !
        0x00, 0x89, 0xe2, 0xda, 0x48, 0x17, 0x29, 0xb5, 0x08, 0x00, 0x00, 0x00, 0x00, 0x20, 0x59, 0x27, //....H.)...... Y' !
        0x3f, 0x08, 0xd0, 0xc2, 0xe4, 0x75, 0x66, 0x10, 0x49, 0x7b, 0xbd, 0x8d, 0xf7                    //?....uf.I{... !]
    };

    RED_CHECK_EQUAL(true, buf.next(TpduBuffer::CREDSSP));
    RED_CHECK_EQUAL(buf.remaining(), 443);
    RED_CHECK_HMEM(authenticate, buf.current_pdu_buffer());

    std::vector<uint8_t> pubauthkey{
        0x30, 0x29, 0xa0, 0x03, 0x02, 0x01, 0x06, 0xa3, 0x22, 0x04, 0x20, 0x01, 0x00, 0x00, 0x00, 0xa2, //0)......". .....
        0xe0, 0x5b, 0x50, 0x97, 0x8e, 0x99, 0x27, 0x00, 0x00, 0x00, 0x00, 0xdc, 0xa7, 0x0b, 0xfe, 0x37, //.[P...'........7
        0x45, 0x3d, 0x1b, 0x05, 0x15, 0xce, 0x56, 0x0a, 0x54, 0xa1, 0xf1,                     //E=....V.T..
    };

    logtrans.send(pubauthkey);

    std::vector<uint8_t>  ts_credentials{
        0x30, 0x5c, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa2, 0x55, 0x04, 0x53, 0x01, 0x00, 0x00, 0x00, 0xaf, // 0.......U.S.....
        0xad, 0x46, 0x2a, 0x6a, 0x9d, 0xf7, 0x88, 0x01, 0x00, 0x00, 0x00, 0xd5, 0x4f, 0xc8, 0xd0, 0xbd, // .F*j........O...
        0x89, 0x60, 0xe0, 0x71, 0x60, 0x31, 0x7a, 0xcc, 0xec, 0xc5, 0xbf, 0x23, 0x4b, 0xe5, 0xf9, 0xa5, // .`.q`1z....#K...
        0x8c, 0x21, 0x66, 0xa6, 0x78, 0xda, 0xd1, 0xbd, 0xef, 0xa4, 0xfd, 0x47, 0xa6, 0xf1, 0x56, 0xa5, // .!f.x......G..V.
        0xd9, 0x52, 0x72, 0x92, 0xfa, 0x41, 0xa5, 0xb4, 0x9d, 0x94, 0xfb, 0x0e, 0xe2, 0x61, 0xba, 0xfc, // .Rr..A.......a..
        0xd5, 0xf3, 0xa7, 0xb5, 0x33, 0xd5, 0x62, 0x8d, 0x93, 0x18, 0x54, 0x39, 0x8a, 0xe7              // ....3.b...T9..
    };

    RED_CHECK_EQUAL(true, buf.next(TpduBuffer::CREDSSP));
    RED_CHECK_EQUAL(buf.remaining(), 94);

    RED_CHECK_HMEM(ts_credentials, buf.current_pdu_buffer());
    RED_CHECK_EQUAL(false, buf.next(TpduBuffer::CREDSSP));
    RED_CHECK_EQUAL(buf.remaining(), 0);
}

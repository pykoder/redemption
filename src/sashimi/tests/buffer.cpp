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

   Product name: sashimi a FLOSS SSH proxy
   Copyright (C) Wallix 2014
   Author(s): Christophe Grosjean

   Unit test to buffer object
   Using lib boost test functions
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestBuffer
#include <boost/test/auto_unit_test.hpp>

//#include "string.hpp"
#include "libssh/libssh.h"

#include "buffer.hpp"

BOOST_AUTO_TEST_CASE(TestInputArrayOneBlock)
{
    ssh_buffer_struct buf;
    buf.out_blob("012345678910", 13);
    char result[13] = {};
    char expected[13] = "012345678910";
    buf.buffer_get_data(result, 13);
    BOOST_CHECK(0 == memcmp(result, expected, 13));
}

BOOST_AUTO_TEST_CASE(TestInputArrayTwo)
{
    ssh_buffer_struct buf;
    buf.out_blob("0123", 4);
    buf.out_blob("45678910", 9);

    char result[7] = {};
    char expected[8] = "0123456"; // we will ignore final 0
    buf.buffer_get_data(result, 7);
    BOOST_CHECK(0 == memcmp(result, expected, 7));

    char result1[6] = {};
    char expected1[6] = "78910";
    buf.buffer_get_data(result1, 6);
    BOOST_CHECK(0 == memcmp(result1, expected1, 6));

}

BOOST_AUTO_TEST_CASE(TestInputArrayU64)
{
    ssh_buffer_struct buf;

    // put in network order
    buf.out_uint64_be(0x0123456789ABCDEF);
    buf.out_uint64_le(0x0123456789ABCDEF);
    buf.out_uint64_le(0x0123456789ABCDEF);

    uint64_t result2 = buf.in_uint64_be();
    BOOST_CHECK_EQUAL(0x0123456789ABCDEF, result2);
    
    uint64_t result1 = buf.in_uint64_le();
    BOOST_CHECK_EQUAL(0x0123456789ABCDEF, result1);

    uint64_t result = buf.in_uint64_le();
    BOOST_CHECK_EQUAL(0x0123456789ABCDEF, result);
    
}

BOOST_AUTO_TEST_CASE(TestSSHString1)
{
    ssh_buffer_struct buf;
    SSHString tmp(10);
    memcpy(tmp.data.get(), "ABCDEFGHI", 10);
    buf.out_uint32_be(tmp.size);
    buf.out_blob(tmp.data.get(), tmp.size);
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */

/*
 * buffer.hpp
 *
 * This file is part of the SaSHimi Library
 *
 * Copyright (c) 2014 by Christophe Grosjean, Meng Tan
 *
 * The SaSHimi Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 
 */

#ifndef STRING_HPP_
#define STRING_HPP_

#include <memory>

enum {
    ERROR_NOT_ENOUGH_DATA = 0
};

#include <memory.h>

struct SSHString
{
    uint32_t size;
    std::unique_ptr<uint8_t[]> data;

    SSHString(uint32_t size) 
        : size(size)
        , data(new uint8_t[size+1])
    {
    }  

    SSHString(int size) 
        : size(static_cast<uint32_t>(size))
        , data(new uint8_t[this->size+1])
    {
    }  

    SSHString(const char * str) 
        : size(strlen(str))
        , data([](const char * str, uint32_t size){
            uint8_t * tmp = new uint8_t[size+1];
            memcpy(tmp, str, size);
            return tmp;
        }(str, this->size))
    {
    }  

    SSHString(const char * str, int size) 
        : size(size)
        , data([](const char * str, uint32_t size){
            uint8_t * tmp = new uint8_t[size+1];
            memcpy(tmp, str, size);
            return tmp;
        }(str, size))
    {
    }  

    SSHString(const uint8_t * str, int size) 
        : size(size)
        , data([](const char * str, uint32_t size){
            uint8_t * tmp = new uint8_t[size+1];
            memcpy(tmp, str, size);
            return tmp;
        }(reinterpret_cast<const char *>(str), size))
    {
    }  

    const char * const cstr() const
    {
        this->data.get()[this->size] = 0;
        return reinterpret_cast<char *>(this->data.get());
    }

};

#endif

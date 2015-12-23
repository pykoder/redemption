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

   log file including syslog
*/

#ifndef _REDEMPTION_UTILS_LOG_HPP_
#define _REDEMPTION_UTILS_LOG_HPP_

#include <string.h>

#define REDOC(x)

// These are used to help coverage chain when function length autodetection (using ctags and gcov) fails

#ifndef VERBOSE
#define TODO(x)
#else
#define DO_PRAGMA(x) _Pragma (#x)
#define TODO(x) DO_PRAGMA(message ("TODO - " x))
#endif

// -Wno-null-dereference and clang++
namespace aux_ {
    class null_pointer
    {
        void * ptr;

    public:
        null_pointer()
        : ptr(nullptr)
        {}

        void * get() const
        { return this->ptr; }
    };
}
#define BOOM (*reinterpret_cast<int*>(aux_::null_pointer().get())=1)

// REDASSERT behave like assert but instaed of calling abort it triggers a segfault
// This is handy to get stacktrace while debugging.
#ifdef NDEBUG
#define REDASSERT(x)
#else
# ifdef LOGPRINT
#  include <cassert>
#  define REDASSERT(x) assert(x)
# else
#  define REDASSERT(x) if(!(x)){BOOM;}
# endif
#endif

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

typedef struct _code {
    const char *c_name;
    int c_val;
} CODE;

#ifdef LOGPRINT
#define LOG LOGPRINT__REDEMPTION__INTERNAL
#elif defined(LOGNULL)
#define LOG LOGNULL__REDEMPTION__INTERNAL
#else
#define LOG LOGSYSLOG__REDEMPTION__INTERNAL
#endif

// LOG_EMERG      system is unusable
// LOG_ALERT      action must be taken immediately
// LOG_CRIT       critical conditions
// LOG_ERR        error conditions
// LOG_WARNING    warning conditions
// LOG_NOTICE     normal, but significant, condition
// LOG_INFO       informational message
// LOG_DEBUG      debug-level message

static inline void LOGSYSLOG__REDEMPTION__INTERNAL(int priority, const char *format, ...)
{
    const CODE prioritynames[] =
    {
        { "EMERG", LOG_EMERG },
        { "ALERT", LOG_ALERT },
        { "CRIT", LOG_CRIT },
        { "ERR", LOG_ERR },
        { "WARNING", LOG_WARNING },
        { "NOTICE", LOG_NOTICE },
        { "INFO", LOG_INFO },
        { "DEBUG", LOG_DEBUG },
        { nullptr, -1 }
    };
    char message[8192];
    va_list vl;
    va_start (vl, format);
    vsnprintf(message, 8191, format, vl);
    va_end(vl);
    syslog(priority, "%s (%d/%d) -- %s", prioritynames[priority].c_name, getpid(), getpid(), message);
}

static inline void LOGPRINT__REDEMPTION__INTERNAL(int priority, const char *format, ...)
{
    const CODE prioritynames[] =
    {
        { "EMERG", LOG_EMERG },
        { "ALERT", LOG_ALERT },
        { "CRIT", LOG_CRIT },
        { "ERR", LOG_ERR },
        { "WARNING", LOG_WARNING },
        { "NOTICE", LOG_NOTICE },
        { "INFO", LOG_INFO },
        { "DEBUG", LOG_DEBUG },
        { nullptr, -1 }
    };
    char message[8192];
    va_list vl;
    va_start (vl, format);
    vsnprintf(message, 8191, format, vl);
    va_end(vl);
    printf("%s (%d/%d) -- %s\n", prioritynames[priority].c_name, getpid(), getpid(), message);
}

static inline void LOGNULL__REDEMPTION__INTERNAL(int priority, const char *format, ...)
{
    (void)priority;
    (void)format;
}

static inline void hexdump(const char * data, size_t size)
{
    char buffer[2048];
    for (size_t j = 0 ; j < size ; j += 16){
        char * line = buffer;
        line += sprintf(line, "%.4x ", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "%.2x ", static_cast<unsigned char>(data[j+i]));
        }
        if (i < 16){
            line += sprintf(line, "%*c", static_cast<unsigned>((16-i)*3), ' ');
        }
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(data[j+i]);
            if ((tmp < ' ') || (tmp > '~')  || (tmp == '\\')){
                tmp = '.';
            }
            line += sprintf(line, "%c", tmp);
        }

        if (line != buffer){
            line[0] = 0;
            LOG(LOG_INFO, "%s", buffer);
            buffer[0]=0;
        }
    }
}

static inline void hexdump(const unsigned char * data, size_t size)
{
    hexdump(reinterpret_cast<const char*>(data), size);
}

static inline void hexdump_d(const char * data, size_t size, unsigned line_length = 16)
{
    char buffer[2048];
    for (size_t j = 0 ; j < size ; j += line_length){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ ", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "0x%.2x, ", static_cast<unsigned char>(data[j+i]));
        }
        if (i < line_length){
            line += sprintf(line, "%*c", static_cast<unsigned>((line_length-i)*3), ' ');
        }

        line += sprintf(line, " // ");

        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(data[j+i]);
            if ((tmp < ' ') || (tmp > '~') || (tmp == '\\')){
                tmp = '.';
            }
            line += sprintf(line, "%c", tmp);
        }

        if (line != buffer){
            line[0] = 0;
            LOG(LOG_INFO, "%s", buffer);
            buffer[0]=0;
        }
    }
}

static inline void hexdump_d(const unsigned char * data, size_t size, unsigned line_length = 16)
{
    hexdump_d(reinterpret_cast<const char*>(data), size, line_length);
}

static inline void hexdump_c(const char * data, size_t size)
{
    char buffer[2048];
    for (size_t j = 0 ; j < size ; j += 16){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "\\x%.2x", static_cast<unsigned char>(data[j+i]));
        }
        line += sprintf(line, "\"");
        if (i < 16){
            line += sprintf(line, "%*c", static_cast<unsigned>((16-i)*4), ' ');
        }
        line += sprintf(line, " //");
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(data[j+i]);
            if ((tmp < ' ') || (tmp > '~') || (tmp == '\\')){
                tmp = '.';
            }
            line += sprintf(line, "%c", tmp);
        }

        if (line != buffer){
            line[0] = 0;
            LOG(LOG_INFO, "%s", buffer);
            buffer[0]=0;
        }
    }
}

static inline void hexdump_c(const unsigned char * data, size_t size)
{
    hexdump_c(reinterpret_cast<const char*>(data), size);
}

static inline void hexdump96_c(const char * data, size_t size)
{
    char buffer[32768];
    const unsigned line_length = 96;
    for (size_t j = 0 ; j < size ; j += line_length){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "\\x%.2x", static_cast<unsigned char>(data[j+i]));
        }
        line += sprintf(line, "\"");
        if (i < line_length){
            line += sprintf(line, "%*c", static_cast<unsigned>((line_length-i)*4), ' ');
        }
        line += sprintf(line, " //");
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(data[j+i]);
            if ((tmp < ' ') || (tmp > '~')){
                tmp = '.';
            }
            line += sprintf(line, "%c", tmp);
        }

        if (line != buffer){
            line[0] = 0;
            LOG(LOG_INFO, "%s", buffer);
            buffer[0]=0;
        }
    }
}

static inline void hexdump96_c(const unsigned char * data, size_t size)
{
    hexdump96_c(reinterpret_cast<const char*>(data), size);
}

static inline void hexdump8_c(const char * data, size_t size)
{
    char buffer[1024];
    const unsigned line_length = 8;
    for (size_t j = 0 ; j < size ; j += line_length){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "\\x%.2x", static_cast<unsigned char>(data[j+i]));
        }
        line += sprintf(line, "\"");
        if (i < line_length){
            line += sprintf(line, "%*c", static_cast<unsigned>((line_length-i)*4), ' ');
        }
        line += sprintf(line, " //");
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(data[j+i]);
            if ((tmp < ' ') || (tmp > '~')){
                tmp = '.';
            }
            line += sprintf(line, "%c", tmp);
        }

        if (line != buffer){
            line[0] = 0;
            LOG(LOG_INFO, "%s", buffer);
            buffer[0]=0;
        }
    }
}

static inline void hexdump8_c(const unsigned char * data, size_t size)
{
    hexdump8_c(reinterpret_cast<const char*>(data), size);
}

#endif

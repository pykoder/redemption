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

// -Wnull-dereference and clang++
namespace { namespace compiler_aux_ {
    inline void * null_pointer()
    { return nullptr; }
} }
#define BOOM (*reinterpret_cast<int*>(compiler_aux_::null_pointer())=1)

// REDASSERT behave like assert but instaed of calling abort it triggers a segfault
// This is handy to get stacktrace while debugging.
#ifdef NDEBUG
#define REDASSERT(x)
#else
//# if defined(LOGPRINT) || defined(REDASSERT_AS_ASSERT)
//#  include <cassert>
//#  define REDASSERT(x) assert(x)
//# else
#  define REDASSERT(x) if(!(x)){BOOM;}
//# endif
#endif

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>


// checked by the compiler
#define LOG_FORMAT_CHECK(...) \
    void(decltype(printf(" " __VA_ARGS__)){})


#ifdef IN_IDE_PARSER
#  define LOG LOGSYSLOG__REDEMPTION__INTERNAL
#  define LOG_SESSION LOGSYSLOG__REDEMPTION__SESSION__INTERNAL

#elif defined(LOGPRINT)
#  define LOG(priority, ...)                                                     \
    LOGCHECK__REDEMPTION__INTERNAL((                                             \
        LOG_FORMAT_CHECK(__VA_ARGS__),                                           \
        LOGPRINT__REDEMPTION__INTERNAL(priority, "%s (%d/%d) -- " __VA_ARGS__),  \
        1                                                                        \
    ))
#  define LOG_SESSION(normal_log, session_log, session_type, type, session_id,   \
        user, device, service, account, priority, ...)                           \
    LOGCHECK__REDEMPTION__INTERNAL((                                             \
        LOG_FORMAT_CHECK(__VA_ARGS__),                                           \
        LOGNULL__REDEMPTION__SESSION__INTERNAL(                                  \
            normal_log,                                                          \
            session_log,                                                         \
            session_type,                                                        \
            type,                                                                \
            session_id,                                                          \
            user,                                                                \
            device,                                                              \
            service,                                                             \
            account                                                              \
        ),                                                                       \
        LOGPRINT__REDEMPTION__INTERNAL(priority, "%s (%d/%d) -- " __VA_ARGS__),  \
        1                                                                        \
    ))

#elif defined(LOGNULL)
#  define LOG(priority, ...) LOG_FORMAT_CHECK(__VA_ARGS__)
#  define LOG_SESSION(normal_log, session_log, session_type, type, session_id,   \
        user, device, service, account, priority, ...)                           \
    LOGCHECK__REDEMPTION__INTERNAL((                                             \
        LOG_FORMAT_CHECK(__VA_ARGS__),                                           \
        LOGNULL__REDEMPTION__SESSION__INTERNAL(                                  \
            normal_log,                                                          \
            session_log,                                                         \
            session_type,                                                        \
            type,                                                                \
            session_id,                                                          \
            user,                                                                \
            device,                                                              \
            service,                                                             \
            account                                                              \
        ),                                                                       \
        1                                                                        \
    ))

#else
#  define LOG(priority, ...)                                                     \
    LOGCHECK__REDEMPTION__INTERNAL((                                             \
        LOG_FORMAT_CHECK(__VA_ARGS__),                                           \
        LOGSYSLOG__REDEMPTION__INTERNAL(priority, "%s (%d/%d) -- " __VA_ARGS__), \
        1                                                                        \
    ))
#  define LOG_SESSION(normal_log, session_log, session_type, type, session_id,   \
        user, device, service, account, priority, format, ...                    \
    )                                                                            \
    LOGCHECK__REDEMPTION__INTERNAL((                                             \
        LOG_FORMAT_CHECK(format, __VA_ARGS__),                                   \
        LOGSYSLOG__REDEMPTION__SESSION__INTERNAL(                                \
            normal_log,                                                          \
            session_log,                                                         \
            session_type, type, session_id,                                      \
            user, device, service, account, priority,                            \
            "%s (%d/%d) -- type='%s'%s" format,                                  \
            "[%s Session] "                                                      \
                "type='%s' "                                                     \
                "session_id='%s' "                                               \
                "user='%s' "                                                     \
                "device='%s' "                                                   \
                "service='%s' "                                                  \
                "account='%s'%s"                                                 \
                format,                                                          \
            ((*format) ? " " : ""),                                              \
            __VA_ARGS__                                                          \
        ), 1)                                                                    \
    )
#endif

namespace {
    // LOG_EMERG      system is unusable
    // LOG_ALERT      action must be taken immediately
    // LOG_CRIT       critical conditions
    // LOG_ERR        error conditions
    // LOG_WARNING    warning conditions
    // LOG_NOTICE     normal, but significant, condition
    // LOG_INFO       informational message
    // LOG_DEBUG      debug-level message

    constexpr const char * const prioritynames[] =
    {
        "EMERG"/*, LOG_EMERG*/,
        "ALERT"/*, LOG_ALERT*/,
        "CRIT"/*, LOG_CRIT*/,
        "ERR"/*, LOG_ERR*/,
        "WARNING"/*, LOG_WARNING*/,
        "NOTICE"/*, LOG_NOTICE*/,
        "INFO"/*, LOG_INFO*/,
        "DEBUG"/*, LOG_DEBUG*/,
        //{ nullptr/*, -1*/ }
    };

    inline void LOGCHECK__REDEMPTION__INTERNAL(int)
    {}

    template<class... Ts>
    void LOGPRINT__REDEMPTION__INTERNAL(int priority, char const * format, Ts const & ... args)
    {
        #ifdef __GNUG__
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat-nonliteral"
        #endif
        printf(format, prioritynames[priority], getpid(), getpid(), args...);
        #ifdef __GNUG__
            #pragma GCC diagnostic pop
        #endif
        puts("");
    }

    template<class... Ts>
    void LOGSYSLOG__REDEMPTION__INTERNAL(int priority, char const * format, Ts const & ... args)
    {
        #ifdef __GNUG__
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat-nonliteral"
        #endif
        syslog(priority, format, prioritynames[priority], getpid(), getpid(), args...);
        #ifdef __GNUG__
            #pragma GCC diagnostic pop
        #endif
    }

    template<class... Ts>
    void LOGSYSLOG__REDEMPTION__SESSION__INTERNAL(
        bool normal_log,
        bool session_log,

        const char * session_type,
        const char * type,
        const char * session_id,
        const char * user,
        const char * device,
        const char * service,
        const char * account,

        int priority,
        const char *format_with_pid,
        const char *format2,
        Ts const & ... args
    ) {
        #ifdef __GNUG__
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat-nonliteral"
        #endif
        if (normal_log) {
            syslog(
                priority, format_with_pid,
                prioritynames[priority], getpid(), getpid(),
                type, args...
            );
        }
        if (session_log) {
            syslog(
                priority, format2,
                session_type,
                type,
                session_id,
                user,
                device,
                service,
                account,
                args...
             );
        }
        #ifdef __GNUG__
            #pragma GCC diagnostic pop
        #endif
    }

    inline void LOGNULL__REDEMPTION__SESSION__INTERNAL(
        bool normal_log,
        bool session_log,
        char const * session_type,
        char const * type,
        char const * session_id,
        char const * user,
        char const * device,
        char const * service,
        char const * account
    ) {
        (void)normal_log;
        (void)session_log;
        (void)session_type;
        (void)type;
        (void)session_id;
        (void)user;
        (void)device;
        (void)service;
        (void)account;
    }
}

namespace {

inline void hexdump(const char * data, size_t size)
{
    char buffer[2048];
    for (size_t j = 0 ; j < size ; j += 16){
        char * line = buffer;
        line += sprintf(line, "%.4x ", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "%.2x ", static_cast<unsigned>(static_cast<unsigned char>(data[j+i])));
        }
        if (i < 16){
            line += sprintf(line, "%*c", static_cast<int>((16-i)*3), ' ');
        }
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(static_cast<unsigned char>(data[j+i]));
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

inline void hexdump(const unsigned char * data, size_t size)
{
    hexdump(reinterpret_cast<const char*>(data), size);
}

inline void hexdump_d(const char * data, size_t size, unsigned line_length = 16)
{
    char buffer[2048];
    for (size_t j = 0 ; j < size ; j += line_length){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ ", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "0x%.2x, ", static_cast<unsigned>(static_cast<unsigned char>(data[j+i])));
        }
        if (i < line_length){
            line += sprintf(line, "%*c", static_cast<int>((line_length-i)*3), ' ');
        }

        line += sprintf(line, " // ");

        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(static_cast<unsigned char>(data[j+i]));
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

inline void hexdump_d(const unsigned char * data, size_t size, unsigned line_length = 16)
{
    hexdump_d(reinterpret_cast<const char*>(data), size, line_length);
}

inline void hexdump_c(const char * data, size_t size)
{
    char buffer[2048];
    for (size_t j = 0 ; j < size ; j += 16){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "\\x%.2x", static_cast<unsigned>(static_cast<unsigned char>(data[j+i])));
        }
        line += sprintf(line, "\"");
        if (i < 16){
            line += sprintf(line, "%*c", static_cast<int>((16-i)*4), ' ');
        }
        line += sprintf(line, " //");
        for (i = 0; i < 16; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(static_cast<unsigned char>(data[j+i]));
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

inline void hexdump_c(const unsigned char * data, size_t size)
{
    hexdump_c(reinterpret_cast<const char*>(data), size);
}

inline void hexdump96_c(const char * data, size_t size)
{
    char buffer[32768];
    const unsigned line_length = 96;
    for (size_t j = 0 ; j < size ; j += line_length){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "\\x%.2x", static_cast<unsigned>(static_cast<unsigned char>(data[j+i])));
        }
        line += sprintf(line, "\"");
        if (i < line_length){
            line += sprintf(line, "%*c", static_cast<int>((line_length-i)*4), ' ');
        }
        line += sprintf(line, " //");
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(static_cast<unsigned char>(data[j+i]));
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

inline void hexdump96_c(const unsigned char * data, size_t size)
{
    hexdump96_c(reinterpret_cast<const char*>(data), size);
}

inline void hexdump8_c(const char * data, size_t size)
{
    char buffer[1024];
    const unsigned line_length = 8;
    for (size_t j = 0 ; j < size ; j += line_length){
        char * line = buffer;
        line += sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
        size_t i = 0;
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            line += sprintf(line, "\\x%.2x", static_cast<unsigned>(static_cast<unsigned char>(data[j+i])));
        }
        line += sprintf(line, "\"");
        if (i < line_length){
            line += sprintf(line, "%*c", static_cast<int>((line_length-i)*4), ' ');
        }
        line += sprintf(line, " //");
        for (i = 0; i < line_length; i++){
            if (j+i >= size){ break; }
            unsigned char tmp = static_cast<unsigned>(static_cast<unsigned char>(data[j+i]));
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

inline void hexdump8_c(const unsigned char * data, size_t size)
{
    hexdump8_c(reinterpret_cast<const char*>(data), size);
}

} // anonymous namespace

#endif

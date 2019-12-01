/*
 *  ____ _             _
 * |_   _(_)_ __  _   _| |    ___   __ _
 *   | | | | '_ \| | | | |   / _ \ / _` | TinyLog for Modern C++
 *   | | | | | | | |_| | |__| (_) | (_| | version 1.4.0
 *   |_| |_|_| |_|\__, |_____\___/ \__, | https://github.com/yanminhui/tinylog
 *                |___/            |___/
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2018-2019 yanminhui <mailto:yanminhui163@163.com>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * ```.cpp
 *
 * using namespace tinylog;
 *
 * int main()
 * {
 *     auto inst = registry::create_logger();
 *
 *     // setup sink:
 *     //   - [w]console_sink
 *     //   - [w]file_sink
 *     //   - [w]u8_file_sink
 *     //   - [w]msvc_sink
 *     //
 *     // @see std::make_shared
 *     auto sk = inst->create_sink<sink::file_sink>("d:\\default.log");
 *     sk->enable_verbose(true);
 *
 *     // filter level.
 *     inst->set_level(info);
 *
 *     // usage:
 *     //   - char
 *     //            |-- lout_<suffix>、lprintf_<suffix>
 *     //            |-- lout(<level>)、lprintf(<level>)
 *     //   - wchar_t
 *     //            |-- wlout_<suffix>、lwprintf_<suffix>
 *     //            |-- wlout(<level>)、lwprintf(<level>)
 *     //
 *     // suffix: t --> trace
 *     //         d --> debug
 *     //         i --> info
 *     //         w --> warn
 *     //         e --> error
 *     //         f --> fatal
 *     //
 *     // @see std::cout \ std::wcout \ printf \ wprintf
 *
 *     // 1
 *     lout_i << "module: pass" << std::endl;
 *
 *     // 2
 *     lout(info) << "module: pass" << std::endl;
 *     lout_if(info, true) << "module: pass" << std::endl;
 *
 *     // 3
 *     lprintf_i("module: %s\n", "pass");
 *
 *     // 4
 *     lprintf(info, "module: %s\n", "pass");
 *     lprintf_if(info, true, "module: %ls\n", "pass");
 * }
 *
 * ```
 */

#ifndef TINYTINYLOG_HPP
#define TINYTINYLOG_HPP

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <atomic>
#include <chrono>
#include <codecvt>
#include <fstream>
#include <functional>
#include <limits>
#include <locale>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>

// Tinylog version.
#define TINYLOG_VERSION_MAJOR 1
#define TINYLOG_VERSION_MINOR 4
#define TINYLOG_VERSION_PATCH 0

/*****************************************************************************/
/* User Customize. */

// Tinylog is built as shared library default, but use headers only is enabled.
//
// @attention Use shared library is recommended that C++ template is
//            instantiated to different instance in shared library.

// #define TINYLOG_USE_HEADER_ONLY 1


// You can enable single thread (a.k.a non-thread-safe), just remove the
// double-slash.

// #define TINYLOG_USE_SINGLE_THREAD 1


// Normally logger is registed at main start (i.e. in the main
// execution thread), but thread-safe is not needed in this case
// (default).

// #define TINYLOG_REGISTRY_THREAD_SAFE 1


// Give more output, available fields: __FILE__, __LINE__, __FUNCTION__.

// #define TINYLOG_CANCEL_VERBOSE 1


// Enables using colors in terminal output (the default). To disable colors
// output, please define macro TINYLOG_DISABLE_CONSOLE_COLOR.

// #define TINYLOG_DISABLE_CONSOLE_COLOR 1


// --- User Customize End ---

#if defined(TINYLOG_USE_SINGLE_THREAD) && defined(TINYLOG_REGISTRY_THREAD_SAFE)
#   undef TINYLOG_REGISTRY_THREAD_SAFE
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define TINYLOG_WINDOWS_API
#else
#   define TINYLOG_POSIX_API
#endif

#if defined(TINYLOG_WINDOWS_API)
#   include <Windows.h>
#else
#   include <unistd.h>
#   include <sys/stat.h>
#endif

#if defined(TINYLOG_USE_HEADER_ONLY) || !defined(TINYLOG_WINDOWS_API)
#   define TINYLOG_API
#elif defined(TINYLOG_EXPORTS) || defined(tinylog_EXPORTS)
#   define TINYLOG_API __declspec(dllexport)
#else
#   define TINYLOG_API __declspec(dllimport)
#   pragma comment(lib, "tinylog.lib")
#endif // TINYLOG_USE_HEADER_ONLY || !TINYLOG_WINDOWS_API

#if defined(TINYLOG_USE_HEADER_ONLY) && defined(TINYLOG_EXPORTS)
#   error must not use TINYLOG_USE_HEADER_ONLY  \
    and TINYLOG_EXPORTS at the same time.
#endif // TINYLOG_USE_HEADER_ONLY && TINYLOG_EXPORTS

#define TINYLOG_CRT_WIDE_(s) L ## s
#define TINYLOG_CRT_WIDE(s)  TINYLOG_CRT_WIDE_(s)

#define TINYLOG_SEPARATOR  " "
#define TINYLOG_SEPARATORW TINYLOG_CRT_WIDE(TINYLOG_SEPARATOR)

#define TINYLOG_TITILE_CHAR  '+'
#define TINYLOG_TITILE_CHARW TINYLOG_CRT_WIDE(TINYLOG_TITILE_CHAR)

#define TINYLOG_DEFAULT  "_TINYLOG_DEFAULT_"
#define TINYLOG_DEFAULTW TINYLOG_CRT_WIDE(TINYLOG_DEFAULT)

#define TINYLOG_LEVEL_TRACE  "TRACE"
#define TINYLOG_LEVEL_DEBUG  "DEBUG"
#define TINYLOG_LEVEL_INFO   "INFO"
#define TINYLOG_LEVEL_WARN   "WARN"
#define TINYLOG_LEVEL_ERROR  "ERROR"
#define TINYLOG_LEVEL_FATAL  "FATAL"
#define TINYLOG_LEVEL_UNKOWN "UNKNOWN"

#define TINYLOG_LEVEL_TRACEW  TINYLOG_CRT_WIDE(TINYLOG_LEVEL_TRACE)
#define TINYLOG_LEVEL_DEBUGW  TINYLOG_CRT_WIDE(TINYLOG_LEVEL_DEBUG)
#define TINYLOG_LEVEL_INFOW   TINYLOG_CRT_WIDE(TINYLOG_LEVEL_INFO)
#define TINYLOG_LEVEL_WARNW   TINYLOG_CRT_WIDE(TINYLOG_LEVEL_WARN)
#define TINYLOG_LEVEL_ERRORW  TINYLOG_CRT_WIDE(TINYLOG_LEVEL_ERROR)
#define TINYLOG_LEVEL_FATALW  TINYLOG_CRT_WIDE(TINYLOG_LEVEL_FATAL)
#define TINYLOG_LEVEL_UNKOWNW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_UNKOWN)

#if defined(__GNUC__)
#   define TINYLOG_FUNCTION __PRETTY_FUNCTION__
#else
#   define TINYLOG_FUNCTION __FUNCTION__
#endif // __GNUC__

// dlout("logger_name", info) << "message" << std::endl;
#if defined(TINYLOG_CANCEL_VERBOSE)

#   define dlprintf(ln, lvl, fmt, ...)                                  \
    for (::tinylog::detail::dlprintf_impl _tl_strm_((ln), (lvl))        \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlwprintf(ln, lvl, fmt, ...)                                 \
    for (::tinylog::detail::dlwprintf_impl _tl_strm_((ln), (lvl))       \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlout(ln, lvl)                                       \
    for (::tinylog::detail::odlstream _tl_strm_((ln), (lvl))    \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#   define wdlout(ln, lvl)                                      \
    for (::tinylog::detail::wodlstream _tl_strm_((ln), (lvl))   \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#else

#   define dlprintf(ln, lvl, fmt, ...)                                  \
    for (::tinylog::detail::dlprintf_d_impl                             \
         _tl_strm_((ln), (lvl), __FILE__, __LINE__                      \
                   , TINYLOG_FUNCTION)                                  \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlwprintf(ln, lvl, fmt, ...)                                 \
    for (::tinylog::detail::dlwprintf_d_impl                            \
         _tl_strm_((ln), (lvl), TINYLOG_CRT_WIDE(__FILE__), __LINE__    \
                   , ::tinylog::a2w(TINYLOG_FUNCTION))                  \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlout(ln, lvl)                               \
    for (::tinylog::detail::odlstream_d                 \
         _tl_strm_((ln), (lvl), __FILE__, __LINE__      \
                   , TINYLOG_FUNCTION)                  \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#   define wdlout(ln, lvl)                                              \
    for (::tinylog::detail::wodlstream_d                                \
         _tl_strm_((ln), (lvl), TINYLOG_CRT_WIDE(__FILE__), __LINE__    \
                   , ::tinylog::a2w(TINYLOG_FUNCTION))                  \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#endif // defined(TINYLOG_CANCEL_VERBOSE)

// lout(info) << "message" << std::endl;
#define lprintf(lvl, fmt, ...)  dlprintf(TINYLOG_DEFAULT                \
                                         , (lvl), (fmt), ##__VA_ARGS__)
#define lwprintf(lvl, fmt, ...) dlwprintf(TINYLOG_DEFAULTW              \
                                          , (lvl), (fmt), ##__VA_ARGS__)
#define lout(lvl)  dlout(TINYLOG_DEFAULT, (lvl))
#define wlout(lvl) wdlout(TINYLOG_DEFAULTW, (lvl))

// dlout_if("logger_name", info, true) << "message" << std::endl;
#define dlprintf_if(ln, lvl, boolexpr, fmt, ...)  if ((boolexpr))       \
        dlprintf((ln), (lvl), (fmt), ##__VA_ARGS__)
#define dlwprintf_if(ln, lvl, boolexpr, fmt, ...) if ((boolexpr))       \
        dlwprintf((ln), (lvl), (fmt), ##__VA_ARGS__)
#define dlout_if(ln, lvl, boolexpr)   if ((boolexpr)) dlout((ln), (lvl))
#define wdlout_if(ln, lvl, boolexpr)  if ((boolexpr)) wdlout((ln), (lvl))

// lout_if(info, true) << "message" << std::endl;
#define lprintf_if(lvl, boolexpr, fmt, ...)  if ((boolexpr))    \
        lprintf((lvl), (fmt), ##__VA_ARGS__)
#define lwprintf_if(lvl, boolexpr, fmt, ...) if ((boolexpr))    \
        lwprintf((lvl), (fmt), ##__VA_ARGS__)
#define lout_if(lvl, boolexpr)  if ((boolexpr)) lout((lvl))
#define wlout_if(lvl, boolexpr) if ((boolexpr)) wlout((lvl))

// lout_i << "message" << std::endl;
#define lprintf_t(fmt, ...)  lprintf(::tinylog::trace, fmt, ##__VA_ARGS__)
#define lwprintf_t(fmt, ...) lwprintf(::tinylog::trace, fmt, ##__VA_ARGS__)
#define lout_t  lout(::tinylog::trace)
#define wlout_t wlout(::tinylog::trace)

#define lprintf_d(fmt, ...)  lprintf(::tinylog::debug, fmt, ##__VA_ARGS__)
#define lwprintf_d(fmt, ...) lwprintf(::tinylog::debug, fmt, ##__VA_ARGS__)
#define lout_d  lout(::tinylog::debug)
#define wlout_d wlout(::tinylog::debug)

#define lprintf_i(fmt, ...)  lprintf(::tinylog::info, fmt, ##__VA_ARGS__)
#define lwprintf_i(fmt, ...) lwprintf(::tinylog::info, fmt, ##__VA_ARGS__)
#define lout_i  lout(::tinylog::info)
#define wlout_i wlout(::tinylog::info)

#define lprintf_w(fmt, ...)  lprintf(::tinylog::warn, fmt, ##__VA_ARGS__)
#define lwprintf_w(fmt, ...) lwprintf(::tinylog::warn, fmt, ##__VA_ARGS__)
#define lout_w  lout(::tinylog::warn)
#define wlout_w wlout(::tinylog::warn)

#define lprintf_e(fmt, ...)  lprintf(::tinylog::error, fmt, ##__VA_ARGS__)
#define lwprintf_e(fmt, ...) lwprintf(::tinylog::error, fmt, ##__VA_ARGS__)
#define lout_e  lout(::tinylog::error)
#define wlout_e wlout(::tinylog::error)

#define lprintf_f(fmt, ...)  lprintf(::tinylog::fatal, fmt, ##__VA_ARGS__)
#define lwprintf_f(fmt, ...) lwprintf(::tinylog::fatal, fmt, ##__VA_ARGS__)
#define lout_f  lout(::tinylog::fatal)
#define wlout_f wlout(::tinylog::fatal)

#if 0
#endif

namespace tinylog
{
namespace detail
{
// Dummy mutex.
struct null_mutex
{
    void lock() {}
    void unlock() {}
    bool try_lock()
    {
        return true;
    }
};

struct time_value
{
    std::time_t tv_sec;
    std::size_t tv_usec;
};

// Get current time.
inline time_value curr_time()
{
    using namespace std::chrono;

    auto const tp          = system_clock::now();
    auto const dtn         = tp.time_since_epoch();
    std::time_t const sec  = duration_cast<seconds>(dtn).count();
    std::size_t const usec = duration_cast<microseconds>(dtn).count() % 1000000;
    return { sec, static_cast<std::size_t>(usec) };
}

// Get current thread id.
inline std::size_t curr_thrd_id()
{
    auto get_thrd_id = []() -> std::size_t
                       {
                           // auto const curr_id = std::this_thread::get_id();
                           // auto gen = std::hash<std::thread::id>();
                           // return static_cast<std::size_t>(gen(curr_id));
                           static std::atomic_size_t id(0u);
                           ++id;
                           return id.load();
                       };

    static thread_local std::size_t const thrd_id = get_thrd_id();
    return thrd_id;
}

// Format date time.
//
// @see strftime
template <class strftimeT, class charT>
std::basic_string<charT>
strftime_impl(strftimeT strftime_cb, charT const* fmt, time_value const& tv)
{
    constexpr auto time_bufsize = 21;
    std::basic_string<charT> time_buffer(time_bufsize, '\0');

    struct tm timeinfo;

#if defined(TINYLOG_WINDOWS_API)
    ::localtime_s(&timeinfo, &tv.tv_sec);
#else
    ::localtime_r(&tv.tv_sec, &timeinfo);
#endif // TINYLOG_WINDOWS_API

    auto const n = strftime_cb(const_cast<charT*>(time_buffer.data())
                               , time_bufsize, fmt, &timeinfo);
    time_buffer.resize(n);

    // append microseconds
    std::basic_string<charT> fmt_s(fmt);
    std::basic_ostringstream<charT> oss;
    if (!fmt_s.empty() && *(fmt_s.rbegin()) == oss.widen('.'))
    {
        oss.imbue(std::locale::classic());
        oss << std::setfill(oss.widen('0')) << std::setw(6) << tv.tv_usec;
    }
    return time_buffer + oss.str();
}

// Ensure l[w]printf's argument is available. Try make compile error,
// because printf can't dump std::string.
// e.g.
//      std::string s("hello");
//      std::printf("%s", s);
inline void ensure_va_args_safe_A() {}
template <class T, class... Args
          , typename std::enable_if<!std::is_class<T>::value
                                    && !std::is_same
                                    <typename std::remove_cv
                                     <typename std::remove_pointer
                                      <typename std::decay<T>::type
                                       >::type>::type, wchar_t>::value
                                    , int>::type = 0>
void ensure_va_args_safe_A(T const&, Args... args)
{
    ensure_va_args_safe_A(args...);
}

inline void ensure_va_args_safe_W() {}
template <class T, class... Args
          , typename std::enable_if<!std::is_class<T>::value
                                    && !std::is_same
                                    <typename std::remove_cv
                                     <typename std::remove_pointer
                                      <typename std::decay<T>::type
                                       >::type>::type, char>::value
                                    , int>::type = 0>
void ensure_va_args_safe_W(T const&, Args... args)
{
    ensure_va_args_safe_W(args...);
}

// Construct string message from avarible arguments.
//
// @see s[w]printf
template <class charT>
struct sprintf_constructor;

template <>
struct sprintf_constructor<char>
{
    using char_type = char;
    using string_t = std::basic_string<char_type>;

    template <class... Args>
    static void construct(string_t& s, string_t const& fmt, Args&&... args)
    {
#if !defined(TINYLOG_CANCEL_VERBOSE)
        ensure_va_args_safe_A(std::forward<Args>(args)...);
#endif
        for (int n = BUFSIZ; true; n += BUFSIZ)
        {
            s.resize(n--);
            auto r = std::snprintf(const_cast<char_type*>(s.data()), n
                                   , fmt.c_str(), args...);
            if (r < 0)
            {
                // TODO: throw system_error
                s.clear();
                break;
            }
            if (r < n)
            {
                s.resize(r);
                break;
            }
        }
    }
};

template <>
struct sprintf_constructor<wchar_t>
{
    using char_type = wchar_t;
    using string_t = std::basic_string<char_type>;

    template <class... Args>
    static void construct(string_t& s, string_t const& fmt, Args&&... args)
    {
#if !defined(TINYLOG_CANCEL_VERBOSE)
        ensure_va_args_safe_W(std::forward<Args>(args)...);
#endif
        for (int n = BUFSIZ; true; n += BUFSIZ)
        {
            s.resize(n--);
            auto r = std::swprintf(const_cast<char_type*>(s.data()), n
                                   , fmt.c_str(), args...);
            if (r < 0)
            {
                // TODO: throw system_error
                s.clear();
                break;
            }
            if (r < n)
            {
                s.resize(r);
                break;
            }
        }
    }
};

// Get file size.
inline std::uintmax_t file_size(std::string const& filename)
{
#if defined(TINYLOG_WINDOWS_API)

    WIN32_FILE_ATTRIBUTE_DATA fad;

    if (!::GetFileAttributesExA(filename.c_str()
                                , ::GetFileExInfoStandard, &fad))
    {
        throw std::system_error(::GetLastError()
                                , std::system_category()
                                , "can't get file size");
    }

    return (static_cast<std::uintmax_t>(fad.nFileSizeHigh)
            << (sizeof(fad.nFileSizeLow) * 8)) + fad.nFileSizeLow;

#else

    struct stat fn_stat;
    if (::stat(filename.c_str(), &fn_stat) != 0
        || !S_ISREG(fn_stat.st_mode))
    {
        throw std::system_error(errno
                                , std::system_category()
                                , "can't get file size");
    }

    return fn_stat.st_size;

#endif // TINYLOG_WINDOWS_API
}

// Moves an existing file or directory, including its children.
inline void file_rename(std::string const& old
                        , std::string const& now)
{
#if defined(TINYLOG_WINDOWS_API)

    if (!::MoveFileExA(old.c_str(), now.c_str()
                       , MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        throw std::system_error(::GetLastError()
                                , std::system_category(), "move file failed");
    }

#else

    if (!::rename(old.c_str(), now.c_str()))
    {
        throw std::system_error(errno
                                , std::system_category(), "move file failed");
    }

#endif
}

// Generate string message.
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +                                                          +
// +                   2018/05/20 15:14:23                    +
// +                                                          +
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <class charT>
std::basic_string<charT>
gen_title(std::basic_string<charT> const& tm_text, charT sep)
{
    constexpr auto wide = 79U;

    // top
    std::basic_ostringstream<charT> oss;
    oss << std::endl;
    for (size_t i = 0; i != wide; ++i)
    {
        oss << sep;
    }
    oss << std::endl;

    // center
    oss << sep << std::setw(wide-1) << sep << std::endl;
    if (tm_text.size() <= wide / 2)
    {
        auto const text_pos = (wide - 2 + tm_text.size()) / 2;
        auto const sep_pos = (wide - 2 - tm_text.size()) / 2 + 1;
        oss << sep
            << std::setw(text_pos) << tm_text
            << std::setw(sep_pos) << sep << std::endl;

        oss << sep << std::setw(wide - 1) << sep << std::endl;
    }

    // bottom
    for (size_t i = 0; i != wide; ++i)
    {
        oss << sep;
    }
    oss << std::endl;
    return oss.str();
}

} // namespace detail

/*****************************************************************************/
/* String Charset Convertion. */

template <typename = void, typename = void>
struct string_traits;

template <>
struct string_traits<void>
{
    template <class To, class From>
    static void convert(To& to, From const& from);
};

using u8string = std::basic_string<char>;

template <>
struct string_traits<u8string>
{
    template <class To, class From>
    static void convert(To& to, From const& from);
};

namespace detail
{

template <class charT>
struct ansi_constructor;

template <>
struct ansi_constructor<char>
{
    using char_type = char;
    using string_t = std::basic_string<char_type>;

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, char_type>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        to = from;
    }

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, wchar_t>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        using from_type = charFT;
        using to_type = char_type;
        using cvt_facet = std::codecvt<from_type, to_type, std::mbstate_t>;

        constexpr std::size_t codecvt_buf_size = BUFSIZ;
        // Perhaps too large, but that's OK.
        // Encodings like shift-JIS need some prefix space
        std::size_t buf_size = from.length() * 4 + 4;
        if (buf_size < codecvt_buf_size)
        {
            buf_size = codecvt_buf_size;
        }
        to.resize(buf_size);

        std::locale loc("");
        auto& cvt = std::use_facet<cvt_facet>(loc);
        do
        {
            from_type const* fb = from.data();
            from_type const* fe = from.data() + from.length();
            from_type const* fn = nullptr;

            to_type* tb = const_cast<to_type*>(to.data());
            to_type* te = const_cast<to_type*>(to.data() + to.length());
            to_type* tn = nullptr;

            std::mbstate_t state = std::mbstate_t();
            auto result = cvt.out(state, fb, fe, fn, tb, te, tn);
            if (result == cvt_facet::ok || result == cvt_facet::noconv)
            {
                to.resize(tn - tb);
                break;
            }
            else if (result == cvt_facet::partial)
            {
                buf_size += codecvt_buf_size;
                to.resize(buf_size);
                continue;
            }
            to.clear();
            break; // failed
        } while (true);
    }
};

template <>
struct ansi_constructor<wchar_t>
{
    using char_type = wchar_t;
    using string_t = std::basic_string<char_type>;

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, char_type>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        to = from;
    }

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, char>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        using from_type = charFT;
        using to_type = char_type;
        using cvt_facet = std::codecvt<to_type, from_type, std::mbstate_t>;

        constexpr std::size_t codecvt_buf_size = BUFSIZ;
        // Perhaps too large, but that's OK
        std::size_t buf_size = from.length() * 3;
        if (buf_size < codecvt_buf_size)
        {
            buf_size = codecvt_buf_size;
        }
        to.resize(buf_size);

        std::locale loc("");
        auto& cvt = std::use_facet<cvt_facet>(loc);
        do
        {
            from_type const* fb = from.data();
            from_type const* fe = from.data() + from.length();
            from_type const* fn = nullptr;

            to_type* tb = const_cast<to_type*>(to.data());
            to_type* te = const_cast<to_type*>(to.data() + to.length());
            to_type* tn = nullptr;

            std::mbstate_t state = std::mbstate_t();
            auto result = cvt.in(state, fb, fe, fn, tb, te, tn);
            if (result == cvt_facet::ok || result == cvt_facet::noconv)
            {
                to.resize(tn - tb);
                break;
            }
            else if (result == cvt_facet::partial)
            {
                buf_size += codecvt_buf_size;
                to.resize(buf_size);
                continue;
            }
            to.clear();
            break; // failed
        } while (true);
    }
};

template <class charT>
struct utf8_constructor;

template <>
struct utf8_constructor<char>
{
    using char_type = char;
    using string_t = std::basic_string<char_type>;

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, char_type>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        using wide_type = wchar_t;
        // ansi --> wide
        std::basic_string<wide_type> ws;
        string_traits<>::convert(ws, from);
        // wide --> utf8
        construct(to, ws);
    }

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, wchar_t>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        // wide --> utf8
        std::wstring_convert<std::codecvt_utf8<charFT>, charFT> cv;
        to = cv.to_bytes(from);
    }
};

template <>
struct utf8_constructor<wchar_t>
{
    using char_type = wchar_t;
    using string_t = std::basic_string<char_type>;

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, char_type>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        to = from;
    }

    template <class charFT
              , typename std::enable_if<std::is_same<charFT, char>::value
                                        , int>::type = 0>
    static void construct(string_t& to, std::basic_string<charFT> const& from)
    {
        using wide_type = char_type;
        // u8 --> wide
        std::wstring_convert<std::codecvt_utf8<wide_type>, wide_type> cv;
        to = cv.from_bytes(from);
    }
};

}  // namespace  detail

// Implement
template <class To, class From>
void string_traits<void>::convert(To& to, From const& from)
{
    using char_type = typename To::value_type;
    detail::ansi_constructor<char_type>::construct(to, from);
}

template <class To, class From>
void string_traits<u8string>::convert(To& to, From const& from)
{
    using char_type = typename To::value_type;
    detail::utf8_constructor<char_type>::construct(to, from);
}

// char to wchar_t
inline std::wstring a2w(std::string const& s)
{
    std::wstring ws;
    string_traits<>::convert(ws, s);
    return ws;
}

/*****************************************************************************/
/* Log Level. */

enum level : std::uint8_t
{
    trace, debug, info, warn, error, fatal
};

template <class charT>
std::basic_string<charT> to_string(level lvl);

template <>
inline std::basic_string<char> to_string(level lvl)
{
    std::basic_string<char> text;
    switch (lvl)
    {
    case level::trace:
        text = TINYLOG_LEVEL_TRACE;
        break;
    case level::debug:
        text = TINYLOG_LEVEL_DEBUG;
        break;
    case level::info:
        text = TINYLOG_LEVEL_INFO;
        break;
    case level::warn:
        text = TINYLOG_LEVEL_WARN;
        break;
    case level::error:
        text = TINYLOG_LEVEL_ERROR;
        break;
    case level::fatal:
        text = TINYLOG_LEVEL_FATAL;
        break;
    default:
        text = TINYLOG_LEVEL_UNKOWN;
        break;
    }
    return  text;
}

template <>
inline std::basic_string<wchar_t> to_string(level lvl)
{
    std::basic_string<wchar_t> text;
    switch (lvl)
    {
    case level::trace:
        text = TINYLOG_LEVEL_TRACEW;
        break;
    case level::debug:
        text = TINYLOG_LEVEL_DEBUGW;
        break;
    case level::info:
        text = TINYLOG_LEVEL_INFOW;
        break;
    case level::warn:
        text = TINYLOG_LEVEL_WARNW;
        break;
    case level::error:
        text = TINYLOG_LEVEL_ERRORW;
        break;
    case level::fatal:
        text = TINYLOG_LEVEL_FATALW;
        break;
    default:
        text = TINYLOG_LEVEL_UNKOWNW;
        break;
    }
    return text;
}

/*****************************************************************************/
/* Record Entry. */

template <class charT>
struct basic_record
{
    using self      = basic_record<charT>;
    using char_type = charT;
    using string_t  = std::basic_string<char_type>;

    explicit basic_record(detail::time_value const& t, level l
                          , std::uintmax_t thrd_id, string_t const& m)
        : tv(t), lvl(l), id(thrd_id), message(m)
    {}

    explicit basic_record(level l, string_t const& m)
        : self(detail::curr_time(), l, detail::curr_thrd_id(), m)
    {}

    explicit basic_record(level l)
        : tv(detail::curr_time()), lvl(l), id(detail::curr_thrd_id())
    {}

    detail::time_value  tv;
    level               lvl;
    std::uintmax_t      id;
    string_t            message;
};

using record  = basic_record<char>;
using wrecord = basic_record<wchar_t>;

template <class charT>
struct basic_record_d : public basic_record<charT>
{
    using base      = basic_record<charT>;
    using self      = basic_record_d<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

    explicit basic_record_d(detail::time_value const& t, level l
                            , std::uintmax_t thrd_id, string_t const& m
                            , string_t const& fn, std::size_t ln
                            , string_t const& fun)
        : base(t, l, thrd_id, m), file(fn), line(ln), func(fun)
    {}

    explicit basic_record_d(level l, string_t const& m
                            , string_t const& fn, std::size_t ln
                            , string_t const& fun)
        : base(l, m), file(fn), line(ln), func(fun)
    {}

    explicit basic_record_d(level l
                            , string_t const& fn, std::size_t ln
                            , string_t const& fun)
        : base(l), file(fn), line(ln), func(fun)
    {}

    string_t        file;
    std::size_t     line;
    string_t        func;
};

using record_d = basic_record_d<char>;
using wrecord_d = basic_record_d<wchar_t>;

/*****************************************************************************/
/* Build Layout: Convert record entry to log message string. */

namespace detail
{

template <class deriveT, class charT>
struct layout_constructor_base
{
    using derive     = deriveT;
    using char_type  = charT;
    using string_t   = std::basic_string<char_type>;
    using ostream_t  = std::basic_ostringstream<char_type>;
    using record     = basic_record<char_type>;
    using record_d   = basic_record_d<char_type>;

    static void construct(string_t& s, record const& r
                          , string_t& cache, bool /*verbose*/)
    {
        ostream_t strm;
        strm.imbue(std::locale::classic());
        derive::record_prefix(strm, r, cache);
        derive::record_suffix(strm, r, cache);
        s = strm.str();
    }

    static void construct(string_t& s, record_d const& r
                          , string_t& cache, bool verbose)
    {
        ostream_t strm;
        strm.imbue(std::locale::classic());
        derive::record_prefix(strm, r, cache);
        if (verbose)
        {
            derive::record_debug(strm, r, cache);
        }
        derive::record_suffix(strm, r, cache);
        s = strm.str();
    }

protected:
    //
    // [format] time (file, line, func) [level] #id message
    //

    // [prefix] time
    static void record_prefix(ostream_t& strm
                              , record const& r, string_t& /*cache*/)
    {
        format_time(strm, r);
    }

    // [suffix] [level] #id message
    static void record_suffix(ostream_t& strm
                              , record const& r, string_t& cache);

    // [debug] (file, line, func)
    static void record_debug(ostream_t& strm
                             , record_d const& r, string_t& cache);

protected:
    static void format_time(ostream_t& strm, record const& r)
    {
        struct tm ti;
#if defined(TINYLOG_WINDOWS_API)
        ::localtime_s(&ti, &r.tv.tv_sec);
#else
        ::localtime_r(&r.tv.tv_sec, &ti);
#endif // TINYLOG_WINDOWS_API
        strm << std::setfill(strm.widen('0'))
             << std::setw(4) << (ti.tm_year + 1900)
             << strm.widen('-') << std::setw(2) << (ti.tm_mon + 1)
             << strm.widen('-') << std::setw(2) << ti.tm_mday
             << strm.widen(' ') << std::setw(2) << ti.tm_hour
             << strm.widen(':') << std::setw(2) << ti.tm_min
             << strm.widen(':') << std::setw(2) << ti.tm_sec
             << strm.widen('.') << std::setw(6) << r.tv.tv_usec;
    }

    static void end_with(ostream_t& strm, char_type delimiter)
    {
        auto const s = strm.str();
        if (s.empty())
        {
            strm << delimiter;
            return ;
        }
        if (*s.crbegin() != delimiter)
        {
            strm << delimiter;
        }
    }
};

template <class charT>
struct layout_constructor;

template <>
struct layout_constructor<char>
    : public layout_constructor_base<layout_constructor<char>, char>
{
    friend struct layout_constructor_base<layout_constructor<char>, char>;

    using base = layout_constructor_base<layout_constructor<char>, char>;

    static constexpr auto sep = TINYLOG_SEPARATOR;
    static constexpr auto lf  = '\n';

protected:
    static void record_suffix(ostream_t& strm
                              , record const& r, string_t& /*cache*/)
    {
        strm << sep << "[" << to_string<char_type>(r.lvl) << "]"
             << sep << "#" << r.id
             << sep << r.message;
        end_with(strm, lf);
    }

    static void record_debug(ostream_t& strm
                             , record_d const& r, string_t& /*cache*/)
    {
        strm << sep << "(" << r.file
             << ", " << r.line << ", " << r.func << ")";
    }
};

template <>
struct layout_constructor<wchar_t>
    : public layout_constructor_base<layout_constructor<wchar_t>, wchar_t>
{
    friend struct layout_constructor_base<layout_constructor<wchar_t>
                                          , wchar_t>;

    using base = layout_constructor_base<layout_constructor<wchar_t>
                                         , wchar_t>;

    static constexpr auto sep = TINYLOG_SEPARATORW;
    static constexpr auto lf  = L'\n';

protected:
    static void record_prefix(ostream_t& strm
                              , record const& r, string_t& /*cache*/)
    {
        format_time(strm, r);
    }

    static void record_suffix(ostream_t& strm
                              , record const& r, string_t& /*cache*/)
    {
        strm << sep << L"[" << to_string<char_type>(r.lvl) << L"]"
             << sep << L"#" << r.id
             << sep << r.message;
        end_with(strm, lf);
    }

    static void record_debug(ostream_t& strm
                             , record_d const& r, string_t& /*cache*/)
    {
        strm << sep << L"(" << r.file
             << L", " << r.line << L", " << r.func << L")";
    }
};

template <class charT>
struct endpage_constructor;

template <>
struct endpage_constructor<char>
    : public layout_constructor_base<endpage_constructor<char>, char>
{
    friend struct layout_constructor_base<endpage_constructor<char>, char>;

    using base = layout_constructor_base<endpage_constructor<char>, char>;

    static constexpr auto sep = TINYLOG_SEPARATOR;
    static constexpr auto lf  = '\f';

protected:
    static void record_prefix(ostream_t& strm
                              , record const& r, string_t& cache)
    {
        if (cache.empty())
        {
            format_time(strm, r);
        }
    }

    static void record_suffix(ostream_t& strm
                              , record const& r, string_t& cache)
    {
        if (cache.empty())
        {
            strm << sep << "[" << to_string<char_type>(r.lvl) << "]"
                 << sep << "#" << r.id << sep;
        }

        if (!r.message.empty() && (*r.message.crbegin() == lf))
        {
            strm << r.message.substr(0, r.message.size()-1) << std::endl;
            cache.clear();
            return ;
        }
        else if (cache.empty())
        {
            cache = { lf };
        }
        strm << r.message;
    }

    static void record_debug(ostream_t& strm
                             , record_d const& r, string_t& cache)
    {
        if (cache.empty())
        {
            strm << sep << "(" << r.file
                 << ", " << r.line << ", " << r.func << ")";
        }
    }
};

template <>
struct endpage_constructor<wchar_t>
    : public layout_constructor_base<endpage_constructor<wchar_t>, wchar_t>
{
    friend struct layout_constructor_base<endpage_constructor<wchar_t>
                                          , wchar_t>;

    using base = layout_constructor_base<endpage_constructor<wchar_t>, wchar_t>;

    static constexpr auto sep = TINYLOG_SEPARATORW;
    static constexpr auto lf  = L'\f';

protected:
    static void record_prefix(ostream_t& strm
                              , record const& r, string_t& cache)
    {
        if (cache.empty())
        {
            format_time(strm, r);
        }
    }

    static void record_suffix(ostream_t& strm
                              , record const& r, string_t& cache)
    {
        if (cache.empty())
        {
            strm << sep << L"[" << to_string<char_type>(r.lvl) << L"]"
                 << sep << L"#" << r.id << sep;
        }

        if (!r.message.empty() && (*r.message.crbegin() == lf))
        {
            strm << r.message.substr(0, r.message.size()-1) << std::endl;
            cache.clear();
            return ;
        }
        else if (cache.empty())
        {
            cache = { lf };
        }
        strm << r.message;
    }

    static void record_debug(ostream_t& strm
                             , record_d const& r, string_t& cache)
    {
        if (cache.empty())
        {
            strm << sep << L"(" << r.file
                 << L", " << r.line << L", " << r.func << L")";
        }
    }
};

}  // namespace detail

/*****************************************************************************/
/* Build Formatter：Use layout constructor generate log message string. */

struct default_layout
{
    template <class stringT, class recordT>
    static void to_string(stringT& s, recordT const& r
                          , stringT& cache, bool verbose)
    {
        using char_type = typename stringT::value_type;
        detail::layout_constructor<char_type>::construct(s, r, cache, verbose);
    }
};

// Log record breaks when message contains form feed (i.e. \f).
//
// loading......ok
// e.g.
//   lout(info) << "loading......";
//   lout(info) << "ok\f";
//
// @warn non-thread-safe.
struct endpage_layout
{
    template <class stringT, class recordT>
    static void to_string(stringT& s, recordT const& r
                          , stringT& cache, bool verbose)
    {
        using char_type = typename stringT::value_type;
        detail::endpage_constructor<char_type>::construct(s, r, cache, verbose);
    }
};

template <class charT, class layoutT = default_layout>
struct formatter
{
public:
    using char_type = charT;
    using string_t = std::basic_string<char_type>;

public:
    template <class recordT
              , typename std::enable_if
              <std::is_same<typename recordT::char_type
                            , char_type>::value, int>::type = 0>
    typename recordT::string_t format(recordT const& r, bool verbose)
    {
        typename recordT::string_t s;
        layoutT::to_string(s, r, cache_, verbose);
        return s;
    }

    // String charset convertion is needed.
    template <class recordT
              , typename std::enable_if
              <!std::is_same<typename recordT::char_type
                             , char_type>::value, int>::type = 0>
    typename recordT::string_t format(recordT const& r, bool verbose)
    {
        typename recordT::string_t s, cache;
        string_traits<>::convert(cache, cache_);
        layoutT::to_string(s, r, cache, verbose);
        string_traits<>::convert(cache_, cache);
        return s;
    }

private:
    string_t cache_;
};

/*****************************************************************************/
/* Colors Support in Terminal Output. */

namespace detail
{

enum class foreground
{
#if defined(TINYLOG_WINDOWS_API)

    white       = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
    , cyan      = FOREGROUND_BLUE | FOREGROUND_GREEN
    , green     = FOREGROUND_GREEN
    , yellow    = FOREGROUND_GREEN | FOREGROUND_RED
    , red       = FOREGROUND_RED

#else

    white       = 37
    , cyan      = 36
    , green     = 32
    , yellow    = 33
    , red       = 31

#endif  // TINYLOG_WINDOWS_API
};

enum class background
{
#if defined(TINYLOG_WINDOWS_API)

    white       = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED
    , cyan      = BACKGROUND_BLUE | BACKGROUND_GREEN
    , green     = BACKGROUND_GREEN
    , yellow    = BACKGROUND_GREEN | BACKGROUND_RED
    , red       = BACKGROUND_RED

#else

    white       = 47
    , cyan      = 46
    , green     = 42
    , yellow    = 43
    , red       = 41

#endif  // TINYLOG_WINDOWS_API
};

enum class emphasize
{
#if defined(TINYLOG_WINDOWS_API)

    normal      = 0
    , bold      = FOREGROUND_INTENSITY

#else

    normal      = 22
    , bold      = 1

#endif  // TINYLOG_WINDOWS_API
};

struct rgb
{
    foreground fg;
    background bg;
    emphasize  em;
};

template <class charT>
struct basic_style_impl
{
    using char_type     = charT;
    using string_t      = std::basic_string<char_type>;

    static rgb curr_rgb(bool raw = false)
    {
#if defined(TINYLOG_WINDOWS_API)

        CONSOLE_SCREEN_BUFFER_INFO info;
        auto handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::GetConsoleScreenBufferInfo(handle, &info);

        auto fg = static_cast<foreground>(info.wAttributes & 0x07);
        auto bg = static_cast<background>(info.wAttributes & 0x70);
        auto em = static_cast<emphasize>(info.wAttributes & 0x08);

#else

        auto fg = raw ? static_cast<foreground>(39) : curr_.fg;
        auto bg = raw ? static_cast<background>(49) : curr_.bg;
        auto em = raw ? static_cast<emphasize>(22) : curr_.em;

#endif  // TINYLOG_WINDOWS_API

        if (raw)
        {
            // nothing to do.
        }
        else if (reset_)
        {
            reset_ = false;
        }

        return { fg, bg, em };
    }

    static rgb curr_rgb_raw()
    {
        bool raw = true;
        return curr_rgb(raw);
    }

    static string_t set_rgb(rgb const& c)
    {
#if defined(TINYLOG_WINDOWS_API)

        CONSOLE_SCREEN_BUFFER_INFO info;
        auto handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::GetConsoleScreenBufferInfo(handle, &info);

        info.wAttributes &= 0xFF80;
        using attr_t    = decltype(info.wAttributes);
        auto fg         = static_cast<attr_t>(c.fg);
        auto bg         = (info.wAttributes & 0x70) | static_cast<attr_t>(c.bg);
        auto em         = static_cast<attr_t>(c.em);

        info.wAttributes |= fg;
        info.wAttributes |= bg;
        info.wAttributes |= em;
        ::SetConsoleTextAttribute(handle, info.wAttributes);
        return string_t();

#else

        curr_ = c;

        std::basic_ostringstream<char_type> oss;
        auto calc = [&oss](std::size_t n)
                    {
                        oss << oss.widen('\033') << oss.widen('[')
                            << n << oss.widen('m');
                    };
        calc(static_cast<std::size_t>(curr_.fg));
        calc(static_cast<std::size_t>(curr_.bg));
        calc(static_cast<std::size_t>(curr_.em));

        return oss.str();

#endif  // TINYLOG_WINDOWS_API
    }

    static string_t set_rgb_raw()
    {
        reset_ = true;
        return set_rgb(default_);
    }

private:
    static rgb default_;
    static bool reset_;
    static rgb curr_;
};

template <class charT>
rgb basic_style_impl<charT>::default_ = basic_style_impl<charT>::curr_rgb_raw();

template <class charT>
bool basic_style_impl<charT>::reset_ = true;

template <class charT>
rgb basic_style_impl<charT>::curr_ = basic_style_impl<charT>::curr_rgb_raw();

template <class charT>
void style(std::basic_string<charT>& color, foreground fg)
{
    using impl  = basic_style_impl<charT>;
    auto curr   = impl::curr_rgb();
    curr.fg     = fg;
    color       = impl::set_rgb(curr);
}

template <class charT>
void style(std::basic_string<charT>& color, background bg)
{
    using impl  = basic_style_impl<charT>;
    auto curr   = impl::curr_rgb();
    curr.bg     = bg;
    color       = impl::set_rgb(curr);
}

template <class charT>
void style(std::basic_string<charT>& color, emphasize em)
{
    using impl  = basic_style_impl<charT>;
    auto curr   = impl::curr_rgb();
    curr.em     = em;
    color       = impl::set_rgb(curr);
}

template <class charT>
void style(std::basic_string<charT>& color)
{
    using impl  = basic_style_impl<charT>;
    color       = impl::set_rgb_raw();
}

}  // namespace detail

/*****************************************************************************/
/* Log Sink:
 *   - console_sink
 *   - file_sink
 *   - u8_file_sink
 *   - msvc_sink
 */

#if defined(TINYLOG_USE_SINGLE_THREAD)
using mutex_t = detail::null_mutex;
#else
using mutex_t = std::mutex;
#endif

namespace sink
{

template <class charT>
class basic_sink_base
{
public:
    using char_type = charT;
    using string_t  = std::basic_string<char_type>;

    virtual ~basic_sink_base() = default;

public:
    explicit operator bool() const
    {
        return is_open();
    }
    bool operator!() const
    {
        return !is_open();
    }

    void set_level(level lvl)
    {
        lvl_ = lvl;
    }
    level get_level() const
    {
        return lvl_;
    }

    void enable_verbose(bool enable)
    {
        verbose_ = enable;
    }
    bool is_verbose() const
    {
        return verbose_;
    }

    virtual bool is_open() const = 0;
    virtual void consume(basic_record<char_type> const& r) = 0;
    virtual void consume(basic_record_d<char_type> const& r) = 0;

private:
    level lvl_      = level::trace;
    bool verbose_   = false;
};

template <class charT, class layoutT = default_layout
          , class mutexT = mutex_t
          , class formatterT = formatter<charT, layoutT>>
class basic_sink : public basic_sink_base<charT>
{
public:
    using base      = basic_sink_base<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

    void consume(basic_record<char_type> const& r) override final
    {
        auto msg = fmt_.format(r, base::is_verbose());
        write(r.lvl, msg);
    }

    void consume(basic_record_d<char_type> const& r) override final
    {
        auto msg = fmt_.format(r, base::is_verbose());
        write(r.lvl, msg);
    }

private:
    void write(level lvl, string_t& msg)
    {
        if (base::get_level() > lvl)
        {
            return ;
        }

        before_write(lvl, msg);
        {
            std::lock_guard<mutexT> lock(mtx_);

            before_writing(lvl, msg);
            writing(lvl, msg);
            after_writing(lvl, msg);
        }
        after_write(lvl, msg);
    }

protected:
    virtual void before_write(level /*lvl*/, string_t& /*msg*/)
    {}

    virtual void before_writing(level /*lvl*/, string_t& /*msg*/)
    {}

    virtual void writing(level /*lvl*/, string_t& /*msg*/) = 0;

    virtual void after_writing(level /*lvl*/, string_t& /*msg*/)
    {}

    virtual void after_write(level /*lvl*/, string_t& /*msg*/)
    {}

private:
    mutexT mtx_;
    formatterT fmt_;
};

//----------------|
// Console Sink   |
//----------------|

template <class charT, class layoutT = default_layout
          , class mutexT = mutex_t
          , class formatterT = formatter<charT, layoutT>>
class basic_console_sink
    : public basic_sink<charT, layoutT, mutexT, formatterT>
{
public:
    using base      = basic_sink<charT, layoutT, mutexT, formatterT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

    bool is_open() const override final
    {
        return true;
    }

#if defined(TINYLOG_DISABLE_CONSOLE_COLOR)

protected:
    void writing(level lvl, string_t& msg) override final
    {
        write_line(lvl, msg);
    }

private:
    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, char>::value, int>::type = 0>
    void write_line(level /*lvl*/
                    , std::basic_string<lineCharT> const& line) const
    {
        std::printf("%s", line.c_str());
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, wchar_t>::value, int>::type = 0>
    void write_line(level /*lvl*/
                    , std::basic_string<lineCharT> const& line) const
    {
        std::wprintf(L"%ls", line.c_str());
    }

#else // Enable colors.

public:
    void enable_color(bool enable = true)
    {
        enable_color_ = enable;
    }

protected:
    void writing(level lvl, string_t& msg) override final
    {
        auto d = line_sep<char_type>();
        for (typename string_t::size_type p = 0, q = 0
             ; q != string_t::npos; q = ++p)
        {
            if ((p = msg.find(d, q)) == string_t::npos)
            {
                write_line(lvl, msg.substr(q));
                break;
            }
            write_line(lvl, msg.substr(q, p - q));
            write_line_sep<char_type>();
        }
    }

private:
    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, char>::value, int>::type = 0>
    lineCharT line_sep() const
    {
        return '\n';
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, wchar_t>::value, int>::type = 0>
    lineCharT line_sep() const
    {
        return L'\n';
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, char>::value, int>::type = 0>
    void write_line(level lvl, std::basic_string<lineCharT> const& line) const
    {
        if (enable_color_)
        {
            std::printf("%s", style_beg(lvl).c_str());
            std::printf("%s", line.c_str());
            std::printf("%s", style_end().c_str());
            return ;
        }
        std::printf("%s", line.c_str());
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, wchar_t>::value, int>::type = 0>
    void write_line(level lvl, std::basic_string<lineCharT> const& line) const
    {
        if (enable_color_)
        {
            std::wprintf(L"%ls", style_beg(lvl).c_str());
            std::wprintf(L"%ls", line.c_str());
            std::wprintf(L"%ls", style_end().c_str());
            return ;
        }
        std::wprintf(L"%ls", line.c_str());
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, char>::value, int>::type = 0>
    void write_line_sep() const
    {
        std::printf("%c", line_sep<lineCharT>());
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, wchar_t>::value, int>::type = 0>
    void write_line_sep() const
    {
        std::wprintf(L"%lc", line_sep<lineCharT>());
    }

private:
    string_t style_beg(level lvl) const
    {
        using namespace detail;

        string_t color_text;
        switch (lvl)
        {
        case level::trace:
            style(color_text, foreground::white);
            break;
        case level::debug:
            style(color_text, foreground::cyan);
            break;
        case level::info:
            style(color_text, foreground::green);
            break;
        case level::warn:
            style(color_text, foreground::yellow);
            style(color_text, emphasize::bold);
            break;
        case level::error:
            style(color_text, foreground::red);
            style(color_text, emphasize::bold);
            break;
        case level::fatal:
            style(color_text, foreground::red);
            style(color_text, background::white);
            style(color_text, emphasize::bold);
            break;
        default:
            break;
        }
        return color_text;
    }

    string_t style_end() const
    {
        string_t color_text;
        detail::style(color_text);
        return color_text;
    }

private:
    bool enable_color_ = true;

#endif // TINYLOG_DISABLE_CONSOLE_COLOR

};

using console_sink  = basic_console_sink<char>;
using wconsole_sink = basic_console_sink<wchar_t>;

//----------------|
// File Sink      |
//----------------|

template <class charT, class layoutT = default_layout
          , class mutexT = mutex_t
          , class formatterT = formatter<charT, layoutT>>
class basic_file_sink
    : public basic_sink<charT, layoutT, mutexT, formatterT>
{
public:
    using base      = basic_sink<charT, layoutT, mutexT, formatterT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

    static constexpr std::uintmax_t default_max_file_size
    = 10 * 1024 * 1024;  // 10 MB
    static constexpr std::uintmax_t npos
    = (std::numeric_limits<std::uintmax_t>::max)();

public:
    explicit basic_file_sink(char const* filename
                             , std::uintmax_t max_file_size
                             = default_max_file_size
                             , std::ios_base::openmode mode
                             = std::ios_base::app
                             , std::locale const& loc = std::locale(""))
        : filename_(filename), max_file_size_(max_file_size)
        , ostrm_(filename, mode)
    {
        ostrm_.imbue(loc);
    }

    bool is_open() const override final
    {
        return ostrm_ ? true : false;
    }

protected:
    void before_writing(level /*lvl*/, string_t& msg) override final
    {
        std::uintmax_t const curr_file_size = ostrm_.tellp();
        if (curr_file_size + msg.size() < max_file_size_)
        {
            return ;
        }

        ostrm_.close();
        try
        {
            ::tinylog::detail::file_rename(filename_, filename_ + ".bak");
        }
        catch (...)
        {
            // TODO: Ignore backup file failed.
        }
        ostrm_.clear();
        ostrm_.open(filename_, std::ios_base::out);
    }

    void writing(level /*lvl*/, string_t& msg) override final
    {
        if (!is_open())
        {
            return;
        }
        ostrm_.seekp(0, std::ios_base::end);
        ostrm_.write(msg.data(), msg.size());
        ostrm_.flush();
    }

private:
    std::string filename_;
    std::uintmax_t max_file_size_ = npos;
    std::basic_ofstream<charT> ostrm_;
};

using file_sink = basic_file_sink<char>;
using wfile_sink= basic_file_sink<wchar_t>;

//----------------|
// U8 File Sink   |
//----------------|

template <class charT, class layoutT = default_layout
          , class mutexT = mutex_t
          , class formatterT = formatter<charT, layoutT>>
class basic_u8_file_sink
    : public basic_file_sink<charT, layoutT, mutexT, formatterT>
{
public:
    using base      = basic_file_sink<charT, layoutT, mutexT, formatterT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

public:
    explicit basic_u8_file_sink(char const* filename
                                , std::uintmax_t max_file_size
                                = base::default_max_file_size
                                , std::ios_base::openmode mode
                                = std::ios_base::app
                                , std::locale const& loc
                                = std::locale(std::locale("")
                                              , new std::codecvt_utf8<wchar_t>))
        : base(filename, max_file_size, mode, loc)
    {
    }

protected:
    void before_write(level /*lvl*/, string_t& msg) override final
    {
        // Convert only if string_t is narrow type.
        string_t const from = msg;
        string_traits<u8string>::convert(msg, from);
    }
};

using u8_file_sink = basic_u8_file_sink<char>;
using wu8_file_sink = basic_u8_file_sink<wchar_t>;

//----------------|
// MSVC Sink      |
//----------------|

#if defined(TINYLOG_WINDOWS_API)

template <class charT, class layoutT = default_layout
          , class mutexT = mutex_t
          , class formatterT = formatter<charT, layoutT>>
class basic_msvc_sink
    : public basic_sink<charT, layoutT, mutexT, formatterT>
{
public:
    using base      = basic_sink<charT, layoutT, mutexT, formatterT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

public:
    bool is_open() const override final
    {
        return true;
    }

protected:
    void writing(level /*lvl*/, string_t& msg) override final
    {
        writing_impl(msg);
    }

private:
    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, char>::value, int>::type = 0>
    void writing_impl(std::basic_string<lineCharT> const& line) const
    {
        ::OutputDebugStringA(line.c_str());
    }

    template <class lineCharT, typename std::enable_if
              <std::is_same<lineCharT, wchar_t>::value, int>::type = 0>
    void writing_impl(std::basic_string<lineCharT> const& line) const
    {
        ::OutputDebugStringW(line.c_str());
    }
};

using msvc_sink = basic_msvc_sink<char>;
using wmsvc_sink= basic_msvc_sink<wchar_t>;

#endif  // TINYLOG_WINDOWS_API

}  // namespace sink

/*****************************************************************************/
/* Sink Adapter: Adapt narrow and unicode charactor. */

namespace detail
{

struct sink_adapter_base
{
    explicit operator bool() const
    {
        return is_open();
    }

    bool operator!() const
    {
        return !is_open();
    }

    virtual bool is_open() const = 0;

    virtual void consume(basic_record<char> const& r) = 0;
    virtual void consume(basic_record<wchar_t> const& r) = 0;

    virtual void consume(basic_record_d<char> const& r) = 0;
    virtual void consume(basic_record_d<wchar_t> const& r) = 0;
};

template <class charT>
struct basic_sink_adapter : public sink_adapter_base
{
    using char_type   = typename std::conditional<std::is_same
                                                  <charT, char>::value
                                                  , char, wchar_t>::type;
    using extern_type = typename std::conditional<!std::is_same
                                                  <charT, char>::value
                                                  , char , wchar_t>::type;

    using string_t    = std::basic_string<char_type>;
    using sink_t      = std::shared_ptr<sink::basic_sink_base<char_type>>;

public:
    explicit basic_sink_adapter(sink_t sk) : sink_(sk)
    {
    }

public:
    bool is_open() const override final
    {
        return sink_ && sink_->is_open();
    }

    void consume(basic_record<char_type> const& r) override
    {
        sink_->consume(r);
    }
    void consume(basic_record<extern_type> const& r) override
    {
        using record_t = basic_record<char_type>;

        string_t m;
        string_traits<>::convert(m, r.message);

        record_t to(r.tv, r.lvl, r.id, m);
        sink_->consume(to);
    }

    void consume(basic_record_d<char_type> const& r) override
    {
        sink_->consume(r);
    }
    void consume(basic_record_d<extern_type> const& r) override
    {
        using record_t = basic_record_d<char_type>;

        string_t m;
        string_traits<>::convert(m, r.message);

        string_t fn;
        string_traits<>::convert(fn, r.file);

        string_t fun;
        string_traits<>::convert(fun, r.func);

        record_t to(r.tv, r.lvl, r.id, m, fn, r.line, fun);
        sink_->consume(to);
    }

private:
    sink_t sink_;
};

}  // namesapce detail

/*****************************************************************************/
/* Logger. */

class logger
{
public:
#if defined(TINYLOG_WINDOWS_API)
    using char_type      = wchar_t;
    using extern_type    = char;
#else
    using char_type      = char;
    using extern_type    = wchar_t;
#endif
    using string_t       = std::basic_string<char_type>;
    using xstring_t      = std::basic_string<extern_type>;
    using sink_adapter_t = std::shared_ptr<detail::sink_adapter_base>;

public:
    explicit logger(string_t const& n) : name_(n)
    {
    }

    explicit logger(xstring_t const& n)
    {
        string_traits<>::convert(name_, n);
    }

#if defined(TINYLOG_WINDOWS_API)
    explicit logger() : name_(TINYLOG_DEFAULTW)
    {
    }
#else
    explicit logger() : name_(TINYLOG_DEFAULT)
    {
    }
#endif // TINYLOG_WINDOWS_API

    string_t name() const
    {
        return name_;
    }

    // Priority:
    //   trace < debug < info < warn < error < fatal
    void set_level(level lvl)
    {
        lvl_ = lvl;
    }

    level get_level() const
    {
        return lvl_;
    }

    // Setup log sink.
    //
    // Usage @see std::make_shared(...)
    // e.g.
    //   auto fk = logger::create_sink<sink::wfile_sink>(L"d:\\error.log");
    //   if (!*fk) {
    //       // open file failed.
    //   }
    template <class sinkT, class... Args>
    std::shared_ptr<sinkT> create_sink(Args&&... args)
    {
        auto sk = std::make_shared<sinkT>(std::forward<Args>(args)...);
        return add_sink(sk);
    }

    template <class sinkT, class charT = typename sinkT::char_type>
    std::shared_ptr<sinkT> add_sink(std::shared_ptr<sinkT> sk)
    {
        using char_type = charT;
        assert(sk && "sink instance point must exists");
        auto ska = std::make_shared<detail::basic_sink_adapter<char_type>>(sk);
        sink_adapters_.emplace_back(ska);
        return sk;
    }

    bool consume(level lvl) const
    {
        return lvl >= get_level();
    }

    template <class recordT>
    void push_record(recordT const& r)
    {
        for (auto& sk_adapter : sink_adapters_)
        {
            if (!*sk_adapter)
            {
                // TODO: Log sink is invalid.
                continue;
            }
            sk_adapter->consume(r);
        }
    }

public:
    // Generate log title message string.
    static std::string title(std::string const& text = "TinyLog")
    {
        using namespace detail;
        return gen_title<std::string::value_type>(text, TINYLOG_TITILE_CHAR);
    }

    static std::wstring wtitle(std::wstring const& text = L"TinyLog")
    {
        using namespace detail;
        return gen_title(text, TINYLOG_TITILE_CHARW);
    }

private:
    string_t name_;
    level lvl_ = level::trace;
    std::vector<sink_adapter_t> sink_adapters_;
};

/*****************************************************************************/
/* Registry: Set up a central registry of logger. */

namespace detail
{

#if defined(TINYLOG_REGISTRY_THREAD_SAFE)
template <class mutexT = std::mutex>
#else
template <class mutexT = ::tinylog::detail::null_mutex>
#endif // TINYLOG_REGISTRY_THREAD_SAFE
class registry_impl
{
public:
    using mutex_type    = mutexT;

#if defined(TINYLOG_WINDOWS_API)
    using char_type     = wchar_t;
    using extern_type   = char;
#else
    using char_type     = char;
    using extern_type   = wchar_t;
#endif
    using string_t      = std::basic_string<char_type>;
    using xstring_t     = std::basic_string<extern_type>;
    using logger_t      = logger;
    using logger_ptr    = std::shared_ptr<logger_t>;

public:
    static registry_impl& instance()
    {
        static bool f = false;
        if(!f)
        {
            // This code *must* be called before main() starts, and
            // when only one thread is executing.
            f = true;
            s_inst_ = std::make_shared<registry_impl>();
        }

        // The following line does nothing else than force the
        // instantiation of singleton<T>::create_object, whose
        // constructor is called before main() begins.
        s_create_object_.do_nothing();

#if !defined(TINYLOG_USE_HEADER_ONLY)
        s_inst_->ensure_impl_in_shared_lib();
#endif // !TINYLOG_USE_HEADER_ONLY

        return *s_inst_;
    }

public:
    void set_level(level lvl)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        lvl_ = lvl;
    }

    logger_ptr create_logger(string_t const& logger_name)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        throw_if_exists(logger_name);
        auto p = std::make_shared<logger_t>(logger_name);
        loggers_[logger_name] = p;
        return p;
    }

    logger_ptr create_logger(xstring_t const& logger_name)
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return create_logger(name);
    }

    logger_ptr create_logger()
    {
#if defined(TINYLOG_WINDOWS_API)
        return create_logger(TINYLOG_DEFAULTW);
#else
        return create_logger(TINYLOG_DEFAULT);
#endif
    }

    logger_ptr add_logger(logger_ptr inst)
    {
        assert(inst && "logger instance point must exists");
        auto const name = cvt(inst->name());
        std::lock_guard<mutex_type> lock(mtx_);
        throw_if_exists(name);
        loggers_[name] = inst;
        return inst;
    }

public:
    logger_ptr get_logger(string_t const& logger_name
                          , level filter_lvl = level::trace)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        auto found = loggers_.find(logger_name);
        if (found != loggers_.cend() && lvl_ <= filter_lvl)
        {
            return found->second;
        }
        return nullptr;
    }

    logger_ptr get_logger(xstring_t const& logger_name
                          , level filter_lvl = level::trace)
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return get_logger(name, filter_lvl);
    }

    logger_ptr get_logger(level filter_lvl = level::trace)
    {
#if defined(TINYLOG_WINDOWS_API)
        return get_logger(TINYLOG_DEFAULTW, filter_lvl);
#else
        return get_logger(TINYLOG_DEFAULT, filter_lvl);
#endif
    }

public:
    void erase_logger(string_t const& logger_name)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        loggers_.erase(logger_name);
    }

    void erase_logger(xstring_t const& logger_name)
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return erase_logger(name);
    }

    void erase_logger()
    {
#if defined(TINYLOG_WINDOWS_API)
        return erase_logger(TINYLOG_DEFAULTW);
#else
        return erase_logger(TINYLOG_DEFAULT);
#endif
    }

    void erase_all_logger()
    {
        std::lock_guard<mutex_type> lock(mtx_);
        loggers_.clear();
    }

    registry_impl() = default;
    registry_impl(registry_impl const&) = delete;
    registry_impl& operator=(registry_impl const&) = delete;

private:
    static string_t cvt(string_t const& xs)
    {
        return xs;
    }
    static string_t cvt(xstring_t const& xs)
    {
        string_t s;
        string_traits<>::convert(s, xs);
        return s;
    }

    void throw_if_exists(string_t const& logger_name) const
    {
        if (loggers_.find(logger_name) != loggers_.cend())
        {
            std::string name;
            string_traits<>::convert(name, logger_name);
            name = "logger with name \'" + name + "\' already exists.";
            throw std::system_error(std::make_error_code(std::errc::file_exists)
                                    , name);
        }
    }

private:
    void ensure_impl_in_shared_lib() const;

private:
    struct object_creator
    {
        object_creator()
        {
            // This constructor does nothing more than ensure that instance()
            // is called before main() begins, thus creating the static
            // T object before multithreading race issues can come up.
            registry_impl::instance();
        }
        inline void do_nothing() const
        {
        }
    };

private:
    static std::shared_ptr<registry_impl> s_inst_;
    static object_creator s_create_object_;

private:
    mutable mutex_type mtx_;
    level lvl_ = level::trace;
    std::unordered_map<string_t, logger_ptr> loggers_;
};

template <class mutexT>
std::shared_ptr<registry_impl<mutexT>> registry_impl<mutexT>::s_inst_;

template <class mutexT>
typename registry_impl<mutexT>::object_creator
registry_impl<mutexT>::s_create_object_;

} // namespace detail

class TINYLOG_API registry
{
public:
    using impl        = detail::registry_impl<>;
    using string_t    = impl::string_t;
    using xstring_t   = impl::xstring_t;
    using logger_ptr  = impl::logger_ptr;

public:
    static void set_level(level lvl)
    {
        return impl::instance().set_level(lvl);
    }

    static logger_ptr create_logger(string_t const& logger_name)
    {
        return impl::instance().create_logger(logger_name);
    }

    static logger_ptr create_logger(xstring_t const& logger_name)
    {
        return impl::instance().create_logger(logger_name);
    }

    static logger_ptr create_logger()
    {
        return impl::instance().create_logger();
    }

    static logger_ptr add_logger(logger_ptr inst)
    {
        return impl::instance().add_logger(inst);
    }

public:
    static logger_ptr get_logger(string_t const& logger_name
                                 , level filter_lvl = level::trace)
    {
        return impl::instance().get_logger(logger_name, filter_lvl);
    }

    static logger_ptr get_logger(xstring_t const& logger_name
                                 , level filter_lvl = level::trace)
    {
        return impl::instance().get_logger(logger_name, filter_lvl);
    }

    static logger_ptr get_logger(level filter_lvl = level::trace)
    {
        return impl::instance().get_logger(filter_lvl);
    }

public:
    void erase_logger(string_t const& logger_name)
    {
        return impl::instance().erase_logger(logger_name);
    }

    void erase_logger(xstring_t const& logger_name)
    {
        return impl::instance().erase_logger(logger_name);
    }

    void erase_logger()
    {
        return impl::instance().erase_logger();
    }

    void erase_all_logger()
    {
        return impl::instance().erase_all_logger();
    }
};

/*****************************************************************************/
/* Capture Logging. */

namespace detail
{

template <class charT>
class basic_dlprintf_base
{
public:
    using char_type = charT;
    using string_t  = std::basic_string<char_type>;
    using logger_t  = logger;
    using logger_ptr= std::shared_ptr<logger_t>;

public:
    explicit basic_dlprintf_base(logger_ptr inst)
        : logger_(inst)
    {
    }

    explicit basic_dlprintf_base(string_t const& logger_name
                                 , level filter_lvl)
        : logger_(registry::get_logger(logger_name, filter_lvl))
    {
    }

    explicit operator bool() const
    {
        return is_open();
    }

    bool operator!() const
    {
        return !is_open();
    }

    bool is_open() const
    {
        return logger_ && logger_->consume(get_level());
    }

    virtual level get_level() const = 0;

    virtual bool flush()
    {
        logger_.reset();
        return true;
    }

protected:
    logger_ptr get_logger()
    {
        return logger_;
    }

private:
    logger_ptr logger_;
};

template <class charT>
class basic_dlprintf
    : public basic_dlprintf_base<charT>
{
public:
    using base      = basic_dlprintf_base<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;
    using record_t  = basic_record<char_type>;
    using logger_t  = typename base::logger_t;
    using logger_ptr= typename base::logger_ptr;

public:
    explicit basic_dlprintf(logger_ptr inst, level lvl)
        : base(inst), record_(lvl)
    {
    }

    explicit basic_dlprintf(string_t const& logger_name, level lvl)
        : base(logger_name, lvl), record_(lvl)
    {
    }

    bool operator()(string_t const& fmt)
    {
        record_.message = fmt;
        return true;
    }

    template <class... Args>
    bool operator()(string_t const& fmt, Args&&... args)
    {
        sprintf_constructor<char_type>::construct(
            record_.message
            , fmt.c_str()
            , std::forward<Args>(args)...);
        return true;
    }

    level get_level() const override
    {
        return record_.lvl;
    }

    bool flush() override
    {
        auto inst = base::get_logger();
        if (inst)
        {
            inst->push_record(record_);
        }
        return base::flush();
    }

private:
    record_t record_;
};

using dlprintf_impl  = basic_dlprintf<char>;
using dlwprintf_impl = basic_dlprintf<wchar_t>;

template <class charT>
class basic_dlprintf_d
    : public basic_dlprintf_base<charT>
{
public:
    using base      = basic_dlprintf_base<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;
    using record_t  = basic_record_d<char_type>;
    using logger_t  = typename base::logger_t;
    using logger_ptr= typename base::logger_ptr;

public:
    explicit basic_dlprintf_d(logger_ptr inst, level lvl
                              , string_t const& file
                              , std::size_t line
                              , string_t const& func)
        : base(inst), record_(lvl, file, line, func)
    {
    }

    explicit basic_dlprintf_d(string_t const& logger_name, level lvl
                              , string_t const& file
                              , std::size_t line
                              , string_t const& func)
        : base(logger_name, lvl), record_(lvl, file, line, func)
    {
    }

    bool operator()(string_t const& fmt)
    {
        record_.message = fmt;
        return true;
    }

    template <class... Args>
    bool operator()(string_t const& fmt, Args&&... args)
    {
        sprintf_constructor<char_type>::construct(
            record_.message
            , fmt.c_str()
            , std::forward<Args>(args)...);
        return true;
    }

    level get_level() const override
    {
        return record_.lvl;
    }

    bool flush() override
    {
        auto inst = base::get_logger();
        if (inst)
        {
            inst->push_record(record_);
        }
        return base::flush();
    }

private:
    record_t record_;
};

using dlprintf_d_impl  = basic_dlprintf_d<char>;
using dlwprintf_d_impl = basic_dlprintf_d<wchar_t>;

template<class charT>
class basic_odlstream_base
    : public std::basic_ostream<charT>
{
public:
    using base      = std::basic_ostream<charT>;
    using char_type = charT;
    using string_t  = std::basic_string<char_type>;
    using sbuf_t    = std::basic_stringbuf<char_type>;
    using logger_t  = logger;
    using logger_ptr= std::shared_ptr<logger_t>;

public:
    explicit basic_odlstream_base(logger_ptr inst)
        : base(&stringbuf_), logger_(inst)
    {
    }

    explicit basic_odlstream_base(string_t const& logger_name
                                  , level filter_lvl = level::trace)
        : base(&stringbuf_)
        , logger_(registry::get_logger(logger_name, filter_lvl))
    {
    }

    explicit operator bool() const
    {
        return is_open();
    }

    bool operator!() const
    {
        return !is_open();
    }

    bool is_open() const
    {
        return logger_ && logger_->consume(get_level());
    }

    virtual level get_level() const = 0;

    virtual bool flush()
    {
        logger_.reset();
        return true;
    }

protected:
    sbuf_t& get_sbuf()
    {
        return stringbuf_;
    }

    logger_ptr get_logger()
    {
        return logger_;
    }

private:
    sbuf_t stringbuf_;
    logger_ptr logger_;
};

template<class charT>
class basic_odlstream
    : public basic_odlstream_base<charT>
{
public:
    using base      = basic_odlstream_base<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;
    using sbuf_t    = typename base::sbuf_t;
    using record_t  = basic_record<char_type>;
    using logger_t  = typename base::logger_t;
    using logger_ptr= typename base::logger_ptr;

public:
    explicit basic_odlstream(logger_ptr inst, level lvl)
        : base(inst), record_(lvl)
    {
    }

    explicit basic_odlstream(string_t const& logger_name
                             , level lvl)
        : base(logger_name, lvl), record_(lvl)
    {
    }

    level get_level() const override
    {
        return record_.lvl;
    }

    bool flush() override
    {
        auto inst = base::get_logger();
        if (inst)
        {
            record_.message = base::get_sbuf().str();
            inst->push_record(record_);
        }
        return base::flush();
    }

private:
    record_t record_;
};

using odlstream  = basic_odlstream<char>;
using wodlstream = basic_odlstream<wchar_t>;

template<class charT>
class basic_odlstream_d
    : public basic_odlstream_base<charT>
{
public:
    using base      = basic_odlstream_base<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;
    using sbuf_t    = typename base::sbuf_t;
    using record_t  = basic_record_d<char_type>;
    using logger_t  = typename base::logger_t;
    using logger_ptr= typename base::logger_ptr;

public:
    explicit basic_odlstream_d(logger_ptr inst, level lvl
                               , string_t const& file
                               , std::size_t line
                               , string_t const& func)
        : base(inst)
        , record_(lvl, file, line, func)
    {
    }

    explicit basic_odlstream_d(string_t const& logger_name
                               , level lvl
                               , string_t const& file
                               , std::size_t line
                               , string_t const& func)
        : base(logger_name, lvl)
        , record_(lvl, file, line, func)
    {
    }

    level get_level() const override
    {
        return record_.lvl;
    }

    bool flush() override
    {
        auto inst = base::get_logger();
        if (inst)
        {
            record_.message = base::get_sbuf().str();
            inst->push_record(record_);
        }
        return base::flush();
    }

private:
    record_t record_;
};

using odlstream_d    = basic_odlstream_d<char>;
using wodlstream_d   = basic_odlstream_d<wchar_t>;

}  // namespace detail
}  // namespace tinylog

#endif  // TINYTINYLOG_HPP

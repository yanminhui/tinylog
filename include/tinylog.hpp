/*
 _____ _             _                
|_   _(_)_ __  _   _| |    ___   __ _ 
  | | | | '_ \| | | | |   / _ \ / _` | TinyLog for Modern C++
  | | | | | | | |_| | |__| (_) | (_| | version 1.0.0
  |_| |_|_| |_|\__, |_____\___/ \__, | https://github.com/yanminhui/tinylog
               |___/            |___/ 

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 颜闽辉 <mailto:yanminhui163@163.com>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/**
 * \brief TinyLog: 一个简易的日志工具
 *
 * 特性:
 *     - 支持 l[w]printf 参数格式化及 [w]lout 流化记录;
 *     - 支持 unicode(wchar_t) 及 narrow(char) 输出;
 *     - 支持输出 ansi、utf-8 格式日志文件;
 *     - 继承 basic_sink<> 可定制日志输出槽将日志输出至不同地方;
 *     - 支持 logger::add_sink<> 同时安装多个日志输出槽;
 *     - 支持 STL 容器格式化输出；
 *     - 提供 hexdump 以十六进制显示内容;
 *     - 线程安全;
 *
 * 示例:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~cpp
 * // [main] 主函数
 *
 * using namespace tinylog;
 *
 * // 安装输出槽：接收日志消息
 * //     - [w]console_sink
 * //           终端槽, 不同级别日志以不同颜色区分.
 * //
 * //     - [w]file_sink
 * //           文件槽, 默认以追加的形式记录日志, 当日志文件大小到达阀值,
 * //       将产生一个以 .bak 为后缀的备份, 如果 .bak 文件已经存在将被替换.
 * //
 * //     - [w]u8_file_sink
 * //           UTF-8 文件槽, 产生以 UTF-8 格式编码的日志文件,
 * //     其它行为与 [w]file_sink 一样.
 * //
 * logger::add_sink<sink::wfile_sink>("d:\\default.log");
 *
 * // 过滤日志级别
 * logger::set_level(info);
 *
 * // [usage] 输出日志
 * //    - char
 * //            |-- 缩写 lout_<suffix>、lprintf_<suffix>
 * //            |-- 通用 lout(<level>)、lprintf(<level>)
 * //    - wchar_t
 * //            |-- 缩写 wlout_<suffix>、lwprintf_<suffix>
 * //            |-- 通用 wlout(<level>)、lwprintf(<level>)
 * //
 * // suffix: t --> trace
 * //         d --> debug
 * //         i --> info
 * //         w --> warn
 * //         e --> error
 * //         f --> fatal
 * // 提示: 使用方式 @see std::cout / std::wcout 及 printf / wprintf
 *
 * // 方式1
 * wlout_i << L"module: pass" << std::endl;
 *
 * // 方式2
 * wlout(tinylog::info) << L"module: pass" << std::endl;
 *
 * // 方式3
 * lwprintf_i(L"module: %ls\n", L"pass");
 *
 * // 方式4
 * lwprintf(tinylog::info, L"module: %ls\n", L"pass");
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  @author 颜闽辉
 *  @date   2018-05-20
 *
 *****************************************************************************
 */

#ifndef TINYTINYLOG_HPP
#define TINYTINYLOG_HPP

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <codecvt>
#include <fstream>
#include <functional>
#include <limits>
#include <list>
#include <locale>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <valarray>

// 版本信息
#define TINYLOG_VERSION_MAJOR 1
#define TINYLOG_VERSION_MINOR 0
#define TINYLOG_VERSION_PATCH 0

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

#define TINYLOG_CRT_WIDE_(s) L ## s
#define TINYLOG_CRT_WIDE(s) TINYLOG_CRT_WIDE_(s)

// 间隔符
#define TINYLOG_SEPARATOR " "
#define TINYLOG_SEPARATORW TINYLOG_CRT_WIDE(TINYLOG_SEPARATOR)

#define TINYLOG_TITILE_CHAR '+'
#define TINYLOG_TITILE_CHARW TINYLOG_CRT_WIDE(TINYLOG_TITILE_CHAR)

// 时间格式: 2018/05/20 19:30:27
#define TINYLOG_DATETIME_FMT "%Y/%m/%d %H:%M:%S"
#define TINYLOG_DATETIME_FMTW TINYLOG_CRT_WIDE(TINYLOG_DATETIME_FMT)

// 调试打印格式: 文件 main.cpp, 行 24, 函数 main()
#define TINYLOG_DEBUG_PRINTF_FMT "文件 %s, 行 %u, 函数 %s"
#define TINYLOG_DEBUG_PRINTF_FMTW L"文件 %ls, 行 %u, 函数 %ls"

// 日志级别
#define TINYLOG_LEVEL_TRACE "跟踪"
#define TINYLOG_LEVEL_TRACEW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_TRACE)

#define TINYLOG_LEVEL_DEBUG "调试"
#define TINYLOG_LEVEL_DEBUGW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_DEBUG)

#define TINYLOG_LEVEL_INFO "信息"
#define TINYLOG_LEVEL_INFOW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_INFO)

#define TINYLOG_LEVEL_WARN "警告"
#define TINYLOG_LEVEL_WARNW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_WARN)

#define TINYLOG_LEVEL_ERROR "错误"
#define TINYLOG_LEVEL_ERRORW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_ERROR)

#define TINYLOG_LEVEL_FATAL "严重"
#define TINYLOG_LEVEL_FATALW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_FATAL)

#define TINYLOG_LEVEL_UNKOWN "未知"
#define TINYLOG_LEVEL_UNKOWNW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_UNKOWN)

#ifdef NDEBUG

#   define lprintf(lvl, fmt, ...) \
    ::tinylog::logger::lformat<char>(lvl, fmt, ##__VA_ARGS__)

#   define lwprintf(lvl, fmt, ...) \
    ::tinylog::logger::lformat<wchar_t>(lvl, fmt, ##__VA_ARGS__)

#   define lout(lvl) for (::tinylog::detail::olstream olstrm(lvl) \
        ; olstrm; olstrm.close()) olstrm

#   define wlout(lvl) \
    for (::tinylog::detail::wolstream wolstrm(lvl) \
        ; wolstrm; wolstrm.close()) wolstrm

#else

#   define lprintf(lvl, fmt, ...) \
    do { \
        ::tinylog::detail::debug_holder<char> dh = \
        { TINYLOG_DEBUG_PRINTF_FMT, __FILE__, __LINE__, __FUNCTION__ }; \
        ::tinylog::logger::lformat_d<char>(dh, lvl, fmt, ##__VA_ARGS__); \
    } while (0)

#   define lwprintf(lvl, fmt, ...) \
    do { \
        ::tinylog::detail::debug_holder<wchar_t> dh = \
        { TINYLOG_DEBUG_PRINTF_FMTW \
        , TINYLOG_CRT_WIDE(__FILE__), __LINE__ \
        , ::tinylog::detail::a2w_holder(__FUNCTION__).c_str() }; \
        ::tinylog::logger::lformat_d<wchar_t>(dh, lvl, fmt, ##__VA_ARGS__); \
    } while (0)

#   define lout(lvl) for (::tinylog::detail::olstream_d olstrm(lvl \
            , __FILE__, __LINE__, __FUNCTION__) \
        ; olstrm; olstrm.close()) olstrm

#   define wlout(lvl) \
    for (::tinylog::detail::wolstream_d wolstrm(lvl \
            , TINYLOG_CRT_WIDE(__FILE__), __LINE__ \
         , ::tinylog::detail::a2w_holder(__FUNCTION__).c_str()) \
        ; wolstrm; wolstrm.close()) wolstrm

#endif  // NDEBUG

// 简写别名
#define lprintf_t(fmt, ...) lprintf(::tinylog::trace, fmt, ##__VA_ARGS__)
#define lwprintf_t(fmt, ...) lwprintf(::tinylog::trace, fmt, ##__VA_ARGS__)
#define lout_t lout(::tinylog::trace)
#define wlout_t wlout(::tinylog::trace)

#define lprintf_d(fmt, ...) lprintf(::tinylog::debug, fmt, ##__VA_ARGS__)
#define lwprintf_d(fmt, ...) lwprintf(::tinylog::debug, fmt, ##__VA_ARGS__)
#define lout_d lout(::tinylog::debug)
#define wlout_d wlout(::tinylog::debug)

#define lprintf_i(fmt, ...) lprintf(::tinylog::info, fmt, ##__VA_ARGS__)
#define lwprintf_i(fmt, ...) lwprintf(::tinylog::info, fmt, ##__VA_ARGS__)
#define lout_i lout(::tinylog::info)
#define wlout_i wlout(::tinylog::info)

#define lprintf_w(fmt, ...) lprintf(::tinylog::warn, fmt, ##__VA_ARGS__)
#define lwprintf_w(fmt, ...) lwprintf(::tinylog::warn, fmt, ##__VA_ARGS__)
#define lout_w lout(::tinylog::warn)
#define wlout_w wlout(::tinylog::warn)

#define lprintf_e(fmt, ...) lprintf(::tinylog::error, fmt, ##__VA_ARGS__)
#define lwprintf_e(fmt, ...) lwprintf(::tinylog::error, fmt, ##__VA_ARGS__)
#define lout_e lout(::tinylog::error)
#define wlout_e wlout(::tinylog::error)

#define lprintf_f(fmt, ...) lprintf(::tinylog::fatal, fmt, ##__VA_ARGS__)
#define lwprintf_f(fmt, ...) lwprintf(::tinylog::fatal, fmt, ##__VA_ARGS__)
#define lout_f lout(::tinylog::fatal)
#define wlout_f wlout(::tinylog::fatal)

//--------------|
// 用户可控制   |
//--------------|

// 使用单线程模式
// #define TINYLOG_USE_SINGLE_THREAD 1

// 禁止STL容器日志
// #define TINYLOG_DISABLE_STL_LOGING 1

// 禁止终端输出颜色
// #define TINYLOG_DISABLE_CONSOLE_COLOR 1

namespace tinylog
{

// 日志级别
enum level : std::uint8_t
{
    trace, debug, info, warn, error, fatal
};

//////////////////////////////////////////////////////////////////////////////
//
// 辅助函数
//
//////////////////////////////////////////////////////////////////////////////
namespace detail
{
// 实现 c++14 make_index_sequence
//
// @see https://github.com/hokein/Wiki/wiki/
//      How to unpack a std::tuple to a function with multiple arguments?
//
template <size_t... Indices>
struct indices_holder {};

// Usage: indices_generator<N>::type
template <size_t Index, size_t... Indices>
struct indices_generator
{
    using type = typename indices_generator<Index - 1
        , Index - 1, Indices...>::type;
};

template <size_t... Indices>
struct indices_generator<0, Indices...>
{
    using type = indices_holder<Indices...>;
};

// 伪装成互斥量
struct null_mutex
{
    void lock() {}
    void unlock() {}
    bool try_lock() { return true; }
};

// 实现 strftime 格式化当前时间
// 返回 std::basic_string<charT>
template <class strftimeT, class charT>
std::basic_string<charT>
current_time(strftimeT strftime_cb, charT const* fmt)
{
    constexpr auto time_bufsize = 20;
    std::basic_string<charT> time_buffer(time_bufsize, '\0');

    auto const rawtime = std::time(nullptr);
    struct tm timeinfo;

#if defined(TINYLOG_WINDOWS_API)
    ::localtime_s(&timeinfo, &rawtime);
#else
    ::localtime_r(&rawtime, &timeinfo);
#endif // TINYLOG_WINDOWS_API

    auto const n = strftime_cb(const_cast<charT*>(time_buffer.data())
        , time_bufsize, fmt, &timeinfo);
    time_buffer.resize(n);

    return time_buffer;
}

// 确保 l[w]printf 可变参数有效
//
// 如，可防止以下错误:
//      std::string s("hello");
//      std::printf("%s", s); // printf 不支持 std::string，但又不报错。
//
inline void ensure_va_args_safe_A() {}
template <class T, class... Args
    , typename std::enable_if<!std::is_class<T>::value
        && !std::is_same<typename std::remove_cv
            <typename std::remove_pointer
            <typename std::decay<T>::type>
            ::type>::type, wchar_t>::value
        , int>::type = 0>
void ensure_va_args_safe_A(T const& , Args... args)
{
    ensure_va_args_safe_A(args...);
}

inline void ensure_va_args_safe_W() {}
template <class T, class... Args
    , typename std::enable_if<!std::is_class<T>::value
        && !std::is_same<typename std::remove_cv
            <typename std::remove_pointer
            <typename std::decay<T>::type>
            ::type>::type, char>::value
        , int>::type = 0>
void ensure_va_args_safe_W(T const& , Args... args)
{
    ensure_va_args_safe_W(args...);
}

// char to wchar_t
struct a2w_holder
{
    a2w_holder(char const* s)
    {
        convert_from(s);
    }

    wchar_t const* c_str()
    {
        return ws_.c_str();
    }

private:
    void convert_from(char const* s)
    {
        for (size_t len = BUFSIZ
             ; len < (std::numeric_limits<size_t>::max)()
             ; len += BUFSIZ)
        {
            ws_.resize(len);

#if defined(TINYLOG_WINDOWS_API)

            size_t w_size = static_cast<size_t>(-1);
            if (::mbstowcs_s(&w_size, const_cast<wchar_t*>(ws_.data())
                , len, s, _TRUNCATE) != 0)
            {
                break; // failed
            }

#else

            size_t w_size = ::mbstowcs(const_cast<wchar_t*>(ws_.data())
                , s, len);

#endif  // TINYLOG_WINDOWS_API

            if (w_size == static_cast<size_t>(-1))
            {
                break; // failed
            }
            if (w_size < len)
            {
                ws_.resize(w_size);
                break;
            }
        }
    }

private:
    std::wstring ws_;
};

// 持有调试信息
template <class charT = char>
struct debug_holder
{
    using string_t = std::basic_string<charT>;

    string_t fmt;
    string_t file;
    size_t  line;
    string_t func;
};

// 获取文件大小
inline std::uintmax_t file_size(std::string const& filename)
{
#if defined(TINYLOG_WINDOWS_API)

    WIN32_FILE_ATTRIBUTE_DATA fad;

    if (!::GetFileAttributesExA(filename.c_str()
        , ::GetFileExInfoStandard, &fad))
    {
        throw std::system_error(::GetLastError()
            , std::system_category(), "can't get file size");
    }

    return (static_cast<std::uintmax_t>(fad.nFileSizeHigh)
        << (sizeof(fad.nFileSizeLow) * 8)) + fad.nFileSizeLow;

#else

    struct stat fn_stat;
    if (::stat(filename.c_str(), &fn_stat) != 0
        || !S_ISREG(fn_stat.st_mode))
    {
        throw std::system_error(errno
            , std::system_category(), "can't get file size");
    }

    return fn_stat.st_size;

#endif
}

// 文件重命名: 移动文件
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

// 生成格式标题:
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

    // 上方
    std::basic_ostringstream<charT> oss;
    oss << std::endl;
    for (size_t i = 0; i != wide; ++i)
    {
        oss << sep;
    }
    oss << std::endl;

    // 中间
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

    // 下方
    for (size_t i = 0; i != wide; ++i)
    {
        oss << sep;
    }
    oss << std::endl;
    return oss.str();
}

//////////////////////////////////////////////////////////////////////////////
//
// 日志记录：
//     格式化记录信息
//
//////////////////////////////////////////////////////////////////////////////
template <class charT>
class basic_record
{
public:
    using char_type = charT;
    using string_t = std::basic_string<char_type>;

public:
    // 日志格式标题:
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // +                                                          +
    // +                   2018/05/20 15:14:23                    +
    // +                                                          +
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    static string_t title()
    {
        return detail::gen_title<char_type>(time(), TINYLOG_TITILE_CHAR);
    }

public:
    // e.g. 2018/05/20 12:30:22 [信息] #49560 界面: 刷新磁盘
    template <class... Args>
    static string_t lformat(level lvl, char_type const* fmt, Args&& ...args)
    {
        return prefix(lvl) + TINYLOG_SEPARATOR
            + message(fmt, std::forward<Args>(args)...);
    }

    // e.g. 文件 main.cpp, 行 24, 函数 main()
    template <class... Args>
    static string_t format(char_type const* fmt, Args&& ...args)
    {
        return message(fmt, std::forward<Args>(args)...);
    }

private:
    // e.g. 2018/05/20 19:30:27 [错误] #2076
    static string_t prefix(level lvl)
    {
        auto const sep = TINYLOG_SEPARATOR;
        return time()
            + sep + level_text(lvl)
            + sep + thread_id();
    }

    template <class... Args>
    static string_t message(char_type const* fmt, Args&& ...args)
    {
        ensure_va_args_safe_A(std::forward<Args>(args)...);

        string_t msg;
        for (int n = BUFSIZ; true; n += BUFSIZ)
        {
            msg.resize(n--);
            auto r = std::snprintf(const_cast<char_type*>(msg.data()), n
                , fmt, args...);
            if (r < 0)
            {
                // TODO: throw system_error
                msg.clear();
                break;
            }
            if (r < n)
            {
                msg.resize(r);
                break;
            }
        }

        constexpr auto newline_sep = '\n';
        if (!msg.empty() && *msg.rbegin() != newline_sep)
        {
            msg.push_back(newline_sep);
        }
        return msg;
    }

private:
    static string_t time()
    {
        return current_time(std::strftime, TINYLOG_DATETIME_FMT);
    }

    static string_t level_text(level lvl)
    {
        string_t msg;
        switch (lvl)
        {
        case level::trace:
            msg = TINYLOG_LEVEL_TRACE;
            break;
        case level::debug:
            msg = TINYLOG_LEVEL_DEBUG;
            break;
        case level::info:
            msg = TINYLOG_LEVEL_INFO;
            break;
        case level::warn:
            msg = TINYLOG_LEVEL_WARN;
            break;
        case level::error:
            msg = TINYLOG_LEVEL_ERROR;
            break;
        case level::fatal:
            msg = TINYLOG_LEVEL_FATAL;
            break;
        default:
            msg = TINYLOG_LEVEL_UNKOWN;
            break;
        }
        return "[" + msg + "]";
    }

    static string_t thread_id()
    {
        auto get_thrd_id = []() -> string_t
        {
            auto const curr_id = std::this_thread::get_id();
            std::basic_ostringstream<char_type> oss;
            if (curr_id == std::thread::id())
            {
                oss << "non-executing";
                return oss.str();
            }
            oss << curr_id;
            return oss.str();
        };

        static thread_local string_t const thrd_id = '#' + get_thrd_id();
        return thrd_id;
    }
};

template <>
class basic_record<wchar_t>
{
public:
    using char_type = wchar_t;
    using string_t = std::basic_string<char_type>;

public:
    // 日志格式标题:
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // +                                                          +
    // +                   2018/05/20 15:14:23                    +
    // +                                                          +
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    static string_t title()
    {
        return detail::gen_title<char_type>(time(), TINYLOG_TITILE_CHARW);
    }

public:
    // e.g. 2018/05/20 12:30:22 [信息] #49560 界面: 刷新磁盘
    template <class... Args>
    static string_t lformat(level lvl, char_type const* fmt, Args&& ...args)
    {
        return prefix(lvl) + TINYLOG_SEPARATORW
            + message(fmt, std::forward<Args>(args)...);
    }

    // e.g. 文件 main.cpp, 行 24, 函数 main()
    template <class... Args>
    static string_t format(char_type const* fmt, Args&& ...args)
    {
        return message(fmt, std::forward<Args>(args)...);
    }

private:
    // e.g. 2018/05/20 19:30:27 [错误] #2076
    static string_t prefix(level lvl)
    {
        auto const sep = TINYLOG_SEPARATORW;
        return time()
            + sep + level_text(lvl)
            + sep + thread_id();
    }

    template <class... Args>
    static string_t message(char_type const* fmt, Args&& ...args)
    {
        ensure_va_args_safe_W(std::forward<Args>(args)...);

        string_t msg;
        for (int n = BUFSIZ; true; n += BUFSIZ)
        {
            msg.resize(n--);
            auto r = std::swprintf(const_cast<char_type*>(msg.data()), n
                , fmt, args...);
            if (r < 0)
            {
                // TODO: throw system_error
                msg.clear();
                break;
            }
            if (r < n)
            {
                msg.resize(r);
                break;
            }
        }

        constexpr auto newline_sep = L'\n';
        if (!msg.empty() && *msg.rbegin() != newline_sep)
        {
            msg.push_back(newline_sep);
        }
        return msg;
    }

private:
    static string_t time()
    {
        return current_time(std::wcsftime, TINYLOG_DATETIME_FMTW);
    }

    static string_t level_text(level lvl)
    {
        string_t msg;
        switch (lvl)
        {
        case level::trace:
            msg = TINYLOG_LEVEL_TRACEW;
            break;
        case level::debug:
            msg = TINYLOG_LEVEL_DEBUGW;
            break;
        case level::info:
            msg = TINYLOG_LEVEL_INFOW;
            break;
        case level::warn:
            msg = TINYLOG_LEVEL_WARNW;
            break;
        case level::error:
            msg = TINYLOG_LEVEL_ERRORW;
            break;
        case level::fatal:
            msg = TINYLOG_LEVEL_FATALW;
            break;
        default:
            msg = TINYLOG_LEVEL_UNKOWNW;
            break;
        }
        return L"[" + msg + L"]";
    }

    static string_t thread_id()
    {
        auto get_thrd_id = []() -> string_t
        {
            auto const curr_id = std::this_thread::get_id();
            std::basic_ostringstream<char_type> oss;
            if (curr_id == std::thread::id())
            {
                oss << L"non-executing";
                return oss.str();
            }
            oss << curr_id;
            return oss.str();
        };

        static thread_local string_t const thrd_id = L'#' + get_thrd_id();
        return thrd_id;
    }
};

}  // namesapce detail

//////////////////////////////////////////////////////////////////////////////
//
// 日志输出槽:
//     使用者可以定制，实现属于自己的槽，使日志消息输出到不同对象上。
//
//////////////////////////////////////////////////////////////////////////////
namespace sink
{

// 互斥量类型
#if defined(TINYLOG_USE_SINGLE_THREAD)
using mutex_t = detail::null_mutex;
#else
using mutex_t = std::mutex;
#endif

//
// 抽象基类:
//     使用者可以继承该模板实现，实现属于自己的槽。
//
// @attention 类中的纯虚函数一定要实现。
//
template <class charT>
class basic_sink
{
public:
    using char_type = charT;
    using string_t = std::basic_string<char_type>;

public:
    virtual ~basic_sink() = default;

    explicit operator bool()
    {
        return is_open();
    }

    bool operator!()
    {
        return !is_open();
    }

public:
    // 是否打开准备好接收数据
    virtual bool is_open() = 0;

    // 消费日志消息
    void consume(level lvl, string_t const& msg)
    {
        string_t msg_ex(msg);
        before_consume(lvl, msg_ex);

        // 保护中...
        {
            std::lock_guard<mutex_t> lock(mtx_);

            before_consuming(lvl, msg_ex);
            consuming(lvl, msg_ex);
            after_consuming(lvl, msg_ex);
        }

        after_consume(lvl, msg_ex);
    }

protected:
    // 在输出数据之前可能需要对数据进行转换处理
    virtual void before_consume(level lvl, string_t& msg)
    {
    }

    // 消费日志消息：before_consuming / consuming / after_consuming 互斥中
    //     不要在这里做太多耗时性作业
    //     尽量把工作转移到 before_consume / after_consume
    virtual void before_consuming(level lvl, string_t& msg)
    {
    }

    virtual void consuming(level lvl, string_t& msg) = 0;

    virtual void after_consuming(level lvl, string_t& msg)
    {
    }

    // 在输出数据之后做一些遗后工作
    virtual void after_consume(level lvl, string_t& msg)
    {
    }

private:
    mutex_t mtx_;
};

// 终端槽
#if defined(TINYLOG_DISABLE_CONSOLE_COLOR)

template <class charT>
struct basic_console_sink_base : public basic_sink<charT>
{
    virtual bool is_open()
    {
        return true;
    }
};

#elif defined(TINYLOG_WINDOWS_API)

template <class charT>
class basic_console_sink_base : public basic_sink<charT>
{
public:
    using color_t = std::pair<WORD, WORD>; // <fore_color, back_color>

    static constexpr auto color_bold = FOREGROUND_INTENSITY;
    static constexpr auto color_white = FOREGROUND_RED
        | FOREGROUND_GREEN | FOREGROUND_BLUE;
    static constexpr auto color_cyan = FOREGROUND_GREEN | FOREGROUND_BLUE;
    static constexpr auto color_green = FOREGROUND_GREEN;
    static constexpr auto color_red = FOREGROUND_RED;
    static constexpr auto color_yellow = FOREGROUND_RED | FOREGROUND_GREEN;

public:
    basic_console_sink_base()
    {
        out_handle_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
    }

    virtual ~basic_console_sink_base()
    {
        ::CloseHandle(out_handle_);
    }

    virtual bool is_open()
    {
        return true;
    }

protected:
    virtual void before_consuming(level lvl, string_t& msg)
    {
        auto color = level_color(lvl);
        orig_color_ = set_console_attribs(std::make_pair(color
            , orig_color_.second));
    }

    virtual void after_consuming(level lvl, string_t& msg)
    {
        set_console_attribs(orig_color_);
    }

private:
    WORD level_color(level lvl)
    {
        switch (lvl)
        {
        case level::trace:
            return color_white;
        case level::debug:
            return color_cyan;
        case level::info:
            return color_green;
        case level::warn:
            return color_bold | color_yellow;
        case level::error:
            return color_bold | color_red;
        case level::fatal:
            return color_bold | color_white | BACKGROUND_RED;
        default:
            break;
        }
        return 0;
    }

    // set color and return the orig console attributes (for resetting later)
    color_t set_console_attribs(color_t color)
    {
        CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
        ::GetConsoleScreenBufferInfo(out_handle_, &orig_buffer_info);

        WORD bk_color = orig_buffer_info.wAttributes;
        bk_color &= static_cast<WORD>(~(FOREGROUND_RED
            | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));

        ::SetConsoleTextAttribute(out_handle_, color.first | color.second);
        return  std::make_pair(orig_buffer_info.wAttributes, bk_color);
    }

private:
    HANDLE out_handle_ = 0;
    color_t orig_color_ = std::make_pair(0, 0);
};

#else  // TINYLOG_POSIX_API

namespace posix
{

template <class charT>
struct color
{
    using char_type = charT;
    using string_t = std::basic_string<char_type>;

    static string_t close() { return "\033[0m"; }
    static string_t trace() { return "\033[37m"; }  // white
    static string_t debug() { return "\033[36m"; }  // cyan = green + blue
    static string_t info()  { return "\033[32m"; }   // green
    static string_t warn()  { return "\033[1;33m"; }  // bold + yellow
    static string_t error() { return "\033[1;31m"; }  // bold + red
    // bold + white + bk_red
    static string_t fatal() { return "\033[1;37;41m"; }
};

template <>
struct color<wchar_t>
{
    using char_type = wchar_t;
    using string_t = std::basic_string<char_type>;

    static string_t close() { return L"\033[0m"; }
    static string_t trace() { return L"\033[37m"; }  // white
    static string_t debug() { return L"\033[36m"; } // cyan = green + blue
    static string_t info()  { return L"\033[32m"; }  // green
    static string_t warn()  { return L"\033[1;33m"; }  // bold + yellow
    static string_t error() { return L"\033[1;31m"; }  // bold + red
    // bold + white + bk_red
    static string_t fatal() { return L"\033[1;37;41m"; }
};

}  // namespace posix

template <class charT>
class basic_console_sink_base : public basic_sink<charT>
{
public:
    using char_type = charT;
    using string_t = std::basic_string<char_type>;

    virtual bool is_open()
    {
        return true;
    }

protected:
    virtual void before_consuming(level lvl, string_t& msg)
    {
        msg = level_color(lvl) + msg + posix::color<char_type>::close();
    }

private:
    string_t level_color(level lvl)
    {
        using namespace posix;
        switch (lvl)
        {
        case level::trace:
            return color<char_type>::trace();
        case level::debug:
            return color<char_type>::debug();
        case level::info:
            return color<char_type>::info();
        case level::warn:
            return color<char_type>::warn();
        case level::error:
            return color<char_type>::error();
        case level::fatal:
            return color<char_type>::fatal();
        default:
            break;
        }
        return 0;
    }
};

#endif  // TINYLOG_DISABLE_CONSOLE_COLOR

template <class charT>
class basic_console_sink : public basic_console_sink_base<charT>
{
public:
    using base = basic_console_sink_base<charT>;
    using char_type = typename base::char_type;
    using string_t = typename base::string_t;

public:
    basic_console_sink(std::locale const& loc = std::locale(""))
    {
        std::clog.imbue(loc);
    }

protected:
    virtual void consuming(level lvl, string_t& msg)
    {
        std::clog.write(msg.data(), static_cast<std::streamsize>(msg.size()));
        std::clog.flush();
    }
};

template <>
class basic_console_sink<wchar_t> : public basic_console_sink_base<wchar_t>
{
public:
    basic_console_sink(std::locale const& loc = std::locale(""))
    {
        std::wclog.imbue(loc);
    }

protected:
    virtual void consuming(level lvl, string_t& msg)
    {
        std::wclog.write(msg.data(), msg.size());
        std::wclog.flush();
    }
};

using console_sink = basic_console_sink<char>;
using wconsole_sink = basic_console_sink<wchar_t>;

// 文件槽
template <class charT>
class basic_file_sink : public basic_sink<charT>
{
public:
    using base = basic_sink<charT>;
    using char_type = typename base::char_type;
    using string_t = typename base::string_t;

    static constexpr std::uintmax_t default_max_file_size
        = 10 * 1024 * 1024;  // 10 MB
    static constexpr std::uintmax_t npos
        = (std::numeric_limits<std::uintmax_t>::max)();

public:
    explicit basic_file_sink(char const* filename
        , std::uintmax_t max_file_size = default_max_file_size
        , std::ios_base::openmode mode = std::ios_base::app
        , std::locale const& loc = std::locale(""))
        : filename_(filename), max_file_size_(max_file_size)
        , ostrm_(filename, mode)
    {
        ostrm_.imbue(loc);
    }

    virtual bool is_open()
    {
        return ostrm_ ? true : false;
    }

protected:
    virtual void before_consuming(level lvl, string_t& msg)
    {
        std::uintmax_t const curr_file_size = ostrm_.tellp();
        if (curr_file_size + msg.size() < max_file_size_)
        {
            return ;
        }

        ostrm_.close();

        try {
            detail::file_rename(filename_, filename_ + ".bak");
        } catch (...) { /* 吞下苦果，避免应用异常退出。*/ }

        ostrm_.clear();
        ostrm_.open(filename_, std::ios_base::out);
    }

    virtual void consuming(level lvl, string_t& msg)
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
using wfile_sink = basic_file_sink<wchar_t>;

// 文件槽(utf-8)
template <class charT>
class basic_u8_file_sink : public basic_file_sink<charT>
{
public:
    using base = basic_file_sink<charT>;
    using char_type = typename base::char_type;
    using string_t = typename base::string_t;

    using codecvt_ansi = std::codecvt_byname<wchar_t, charT, std::mbstate_t>;
    using ansi_cvt_t = std::wstring_convert<codecvt_ansi>;
    using u8_cvt_t = std::wstring_convert<std::codecvt_utf8<wchar_t>>;

public:
    explicit basic_u8_file_sink(char const* filename
        , std::uintmax_t max_file_size = base::default_max_file_size
        , std::ios_base::openmode mode = std::ios_base::app
        , std::locale const& loc = std::locale(std::locale("")
            , new std::codecvt_utf8<wchar_t>))
        : base(filename, max_file_size, mode, loc)
    {
    }

protected:
    virtual void before_consume(level lvl, string_t& msg)
    {
#if defined(TINYLOG_WINDOWS_API)

        auto const ws = ansi_cvt_->from_bytes(msg.data()
            , msg.data()+msg.size());

#else

        std::wstring const ws = detail::a2w_holder(msg.c_str()).c_str();

#endif  // TINYLOG_WINDOWS_API

        msg = u8_cvt_.to_bytes(ws);
    }

private:
#if defined(TINYLOG_WINDOWS_API)

    std::shared_ptr<ansi_cvt_t> ansi_cvt_
        = std::make_shared<ansi_cvt_t>(new codecvt_ansi(""));

#endif  // TINYLOG_WINDOWS_API

    u8_cvt_t u8_cvt_;
};

template <>
class basic_u8_file_sink<wchar_t> : public basic_file_sink<wchar_t>
{
public:
    explicit basic_u8_file_sink(char const* filename
        , std::uintmax_t max_file_size = default_max_file_size
        , std::ios_base::openmode mode = std::ios_base::app
        , std::locale const& loc = std::locale(std::locale("")
            , new std::codecvt_utf8<wchar_t>))
        : basic_file_sink(filename, max_file_size, mode, loc)
    {
    }
};

using u8_file_sink = basic_u8_file_sink<char>;
using wu8_file_sink = basic_u8_file_sink<wchar_t>;

}  // namespace sink

namespace detail
{
//
// 输出槽管理:
//     只被 logger 使用，只使用 basic_sink<>.
//
template <class charT>
class sink_manager
{
public:
    using char_type = charT;
    using string_t = std::basic_string<char_type>;
    using sink_t = std::shared_ptr<sink::basic_sink<charT>>;

public:
    static void add_sink(sink_t sink)
    {
        sinks_.emplace_back(sink);
    }

    static void consume(level lvl, string_t const& msg)
    {
        // bool writed = false;
        for (auto& sk : sinks_)
        {
            if (!*sk)
            {
                continue;
            }
            sk->consume(lvl, msg);
            // writed = true;
        }

        // if (!writed)
        // {
        //     sink::basic_console_sink<charT> sk;
        //     sk.consume(lvl, msg);
        //     return;
        // }
    }

private:
    static std::list<sink_t> sinks_;
};

// 全局静态数据
template <class charT>
std::list<typename sink_manager<charT>::sink_t> sink_manager<charT>::sinks_;

}  // namesapce detail

//////////////////////////////////////////////////////////////////////////////
//
// 日志记录器
//
//////////////////////////////////////////////////////////////////////////////
class logger
{
public:
    /** \brief 安装输出槽
     *
     * 使用形式参见 std::make_shared(...)
     *
     * 示例:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.cpp
     * auto file_sink = logger::add_sink<sink::wfile_sink>(L"d:\\error.log");
     * if (!*file_sink) {
     *     // open file failed.
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    template <class T, class... Args>
    static std::shared_ptr<T> add_sink(Args&&... args)
    {
        auto sk = std::make_shared<T>(std::forward<Args>(args)...);
        detail::sink_manager<typename T::char_type>::add_sink(sk);
        return sk;
    }

    /** \brief 过滤日志级别
     *
     * 低于 lvl 级别的日志不输出
     * 优先级:
     *     trace < debug < info < warn < error < fatal
     */
    static void set_level(level lvl)
    {
        level_ = lvl;
    }

    static level get_level()
    {
        return level_;
    }

    static bool consume(level lvl)
    {
        return lvl >= get_level();
    }

public:
    /** \brief 产生一个标题文本
     *
     * 一般在对日志设置完毕后，做为第一条日志记录，
     * 让日志文件看起来有明显的边界。
     */
    static std::string title()
    {
        using record = detail::basic_record<char>;
        return record::title();
    }

    static std::wstring wtitle()
    {
        using record = detail::basic_record<wchar_t>;
        return record::title();
    }

public:
    /** \brief 格式化日志记录
     *
     * 使用形式参见 std::fprintf(...)
     */
    template <class charT, class... Args>
    static void lformat(level lvl, charT const* fmt, Args&& ...args)
    {
        using record = detail::basic_record<charT>;

        if (!consume(lvl))
        {
            return ;
        }

        std::basic_string<charT> const msg
            = record::lformat(lvl, fmt, std::forward<Args>(args)...);

        detail::sink_manager<charT>::consume(lvl, msg);
    }

    template <class charT, class... Args>
    static void lformat_d(detail::debug_holder<charT> const& dh
        , level lvl, charT const* fmt, Args&& ...args)
    {
        using string_t = std::basic_string<charT>;
        using record = detail::basic_record<charT>;

        if (!consume(lvl))
        {
            return ;
        }

        string_t const msg
            = record::lformat(lvl, fmt, std::forward<Args>(args)...);

        string_t const debug_info
            = record::format(dh.fmt.c_str()
                , dh.file.c_str(), dh.line, dh.func.c_str());

        detail::sink_manager<charT>::consume(lvl, msg + debug_info);
    }

private:
    /** \brief 格式化日志记录
     *
     * 使用形式参见 std::printf(...)
     *
     * format 与 lformat 最大的区别仅仅是 lformat 将产生记录的时间、日志级别等信息,
     * 而 format 完全与 std::printf 一致，仅仅格式化字符串参数。
     */
    template <class charT, class... Args>
    static void format(level lvl, charT const* fmt, Args&& ...args)
    {
        using record = detail::basic_record<charT>;

        if (!consume(lvl))
        {
            return ;
        }

        std::basic_string<charT> const msg
            = record::format(fmt, std::forward<Args>(args)...);

        detail::sink_manager<charT>::consume(lvl, msg);
    }

private:
    static level level_;
};

// 全局静态数据
level logger::level_ = trace;

//////////////////////////////////////////////////////////////////////////////
//
//  流化支持
//
//////////////////////////////////////////////////////////////////////////////
namespace detail
{

template <class charT>
inline charT const* default_lformat_str()
{
    return "%s";
}

template <>
inline wchar_t const* default_lformat_str<wchar_t>()
{
    return L"%ls";
}

template <class charT>
inline charT const* debug_format_str()
{
    return TINYLOG_DEBUG_PRINTF_FMT;
}

template <>
inline wchar_t const* debug_format_str<wchar_t>()
{
    return TINYLOG_DEBUG_PRINTF_FMTW;
}

// 流化支持: 日志输出流持有对象
template<class charT>
class basic_olstream : public std::basic_ostream<charT>
{
public:
    using char_type = charT;
    using base = std::basic_ostream<char_type>;
    using sbuf_t = std::basic_stringbuf<char_type>;

    explicit basic_olstream(level lvl)
            : base(&stringbuf_), level_(lvl)
    {
    }

    explicit operator bool()
    {
        return is_open();
    }

    bool operator!()
    {
        return !is_open();
    }

public:
    bool is_open()
    {
        return logger::consume(level_) && open_;
    }

    virtual void close()
    {
        logger::lformat(level_, default_lformat_str<char_type>()
            , stringbuf_.str().c_str());
        open_ = false;
    }

protected:
    sbuf_t stringbuf_;
    bool open_ = true;
    level level_;
};

using olstream = basic_olstream<char>;
using wolstream = basic_olstream<wchar_t>;

// 支持调试信息
template<class charT>
class basic_olstream_d : public basic_olstream<charT>
{
public:
    using base = basic_olstream<charT>;
    using char_type = typename base::char_type;

public:
    explicit basic_olstream_d(level lvl
        , std::basic_string<charT> const& file
        , size_t line
        , std::basic_string<charT> const& func)
            : base(lvl)
    {
        dh_ = { debug_format_str<char_type>(), file, line, func };
    }

    virtual void close()
    {
        logger::lformat_d(dh_, base::level_
            , default_lformat_str<char_type>()
            , base::stringbuf_.str().c_str());
        base::open_ = false;
    }

private:
    detail::debug_holder<char_type> dh_;
};

using olstream_d = basic_olstream_d<char>;
using wolstream_d = basic_olstream_d<wchar_t>;

}  // namespace detail

//////////////////////////////////////////////////////////////////////////////
//
// STL容器流化支持
//
// 输出格式:
//     pair  --> key: value
//     tuple --> (item_0, item_1, item_2)
//     list  --> [item_0, item_1, item_2 ]
//     map   --> { key_0: value_0, key_1: value_1 }
//
//////////////////////////////////////////////////////////////////////////////
namespace detail
{

#define TINYLOG_SPACE ' '
#define TINYLOG_SPACEW TINYLOG_CRT_WIDE(TINYLOG_SPACE)

#define TINYLOG_ELLIPSIS "..."
#define TINYLOG_ELLIPSISW TINYLOG_CRT_WIDE(TINYLOG_ELLIPSIS)

#define TINYLOG_COMMA ','
#define TINYLOG_COMMAW TINYLOG_CRT_WIDE(TINYLOG_COMMA)

#define TINYLOG_COLON ':'
#define TINYLOG_COLONW TINYLOG_CRT_WIDE(TINYLOG_COLON)

#define TINYLOG_LPARENTHESE '('
#define TINYLOG_LPARENTHESEW TINYLOG_CRT_WIDE(TINYLOG_LPARENTHESE)

#define TINYLOG_RPARENTHESE ')'
#define TINYLOG_RPARENTHESEW TINYLOG_CRT_WIDE(TINYLOG_RPARENTHESE)

#define TINYLOG_LBRACKET '['
#define TINYLOG_LBRACKETW TINYLOG_CRT_WIDE(TINYLOG_LBRACKET)

#define TINYLOG_RBRACKET ']'
#define TINYLOG_RBRACKETW TINYLOG_CRT_WIDE(TINYLOG_RBRACKET)

#define TINYLOG_LBRACE '{'
#define TINYLOG_LBRACEW TINYLOG_CRT_WIDE(TINYLOG_LBRACE)

#define TINYLOG_RBRACE '}'
#define TINYLOG_RBRACEW TINYLOG_CRT_WIDE(TINYLOG_RBRACE)

template <class charT>
inline charT space()
{
    return TINYLOG_SPACE;
}

template <>
inline wchar_t space<wchar_t>()
{
    return TINYLOG_SPACEW;
}

template <class charT>
inline std::basic_string<charT> ellipsis()
{
    return TINYLOG_ELLIPSIS;
}

template <>
inline std::basic_string<wchar_t> ellipsis<wchar_t>()
{
    return TINYLOG_ELLIPSISW;
}

template <class charT>
inline charT comma()
{
    return TINYLOG_COMMA;
}

template <>
inline wchar_t comma<wchar_t>()
{
    return TINYLOG_COMMAW;
}

template <class charT>
inline charT colon()
{
    return TINYLOG_COLON;
}

template <>
inline wchar_t colon<wchar_t>()
{
    return TINYLOG_COLONW;
}

template <class charT>
inline charT lparenthese()
{
    return TINYLOG_LPARENTHESE;
}

template <>
inline wchar_t lparenthese()
{
    return TINYLOG_LPARENTHESEW;
}

template <class charT>
inline charT rparenthese()
{
    return TINYLOG_RPARENTHESE;
}

template <>
inline wchar_t rparenthese()
{
    return TINYLOG_RPARENTHESEW;
}

template <class charT>
inline charT lbracket()
{
    return TINYLOG_LBRACKET;
}

template <>
inline wchar_t lbracket()
{
    return TINYLOG_LBRACKETW;
}

template <class charT>
inline charT rbracket()
{
    return TINYLOG_RBRACKET;
}

template <>
inline wchar_t rbracket()
{
    return TINYLOG_RBRACKETW;
}

template <class charT>
inline charT lbrace()
{
    return TINYLOG_LBRACE;
}

template <>
inline wchar_t lbrace()
{
    return TINYLOG_LBRACEW;
}

template <class charT>
inline charT rbrace()
{
    return TINYLOG_RBRACE;
}

template <>
inline wchar_t rbrace()
{
    return TINYLOG_RBRACEW;
}

// std::tuple
template<class Tuple, size_t... Indices, class charT>
inline void print_tuple(Tuple&& t
    , indices_holder<Indices...>
    , std::basic_ostream<charT>& out)
{
    using swallow = int[];
    auto delimiters = [](size_t idx) -> std::basic_string<charT>
    {
        if (idx == 0)
        {
            return std::basic_string<charT>();
        }
        return { comma<charT>(), space<charT>() };
    };

    out << lparenthese<charT>();
    (void)swallow{0, (out << delimiters(Indices) << std::get<Indices>(t), 0)...};
    out << rparenthese<charT>();
}

struct sfinae
{
    using true_t = char;
    using false_t = char[2];
};

// 容器是否有 const_iterator 类型
template <class T>
struct has_const_iterator : private sfinae
{
private:
    template <class Container>
    static true_t& has(typename Container::const_iterator*);

    template <class Container>
    static false_t& has(...);

public:
    static constexpr bool value = sizeof(has<T>(nullptr)) == sizeof(true_t);
};

// 容器是类 map 类型
template <class T>
struct has_kv : private sfinae
{
private:
    template <class Container>
    static true_t& has_key(typename Container::key_type*);

    template <class Container>
    static false_t& has_key(...);

    template <class Container>
    static true_t& has_value(typename Container::value_type*);

    template <class Container>
    static false_t& has_value(...);

public:
    static constexpr bool value
        = (sizeof(has_key<T>(nullptr)) == sizeof(true_t))
        && (sizeof(has_value<T>(nullptr)) == sizeof(true_t));
};

// 容器是否有 begin() 成员
template <typename T>
struct has_begin_iterator: private sfinae
{
private:
    template <typename Container>
    using begin_t = typename Container::const_iterator(Container::*)() const;

    template <typename Container>
    static true_t& has(typename std::enable_if \
        <std::is_same<decltype(static_cast<begin_t<Container>>(&Container::begin))
            , begin_t<Container>>::value>::type*);

    template <typename Container>
    static false_t& has(...);

public:
    static constexpr bool value = sizeof(has<T>(nullptr)) == sizeof(true_t);
};

// 容器是否有 end() 成员
template <typename T>
struct has_end_iterator: private sfinae
{
private:
    template <typename Container>
    using end_t = typename Container::const_iterator(Container::*)() const;

    template <typename Container>
    static true_t& has(typename std::enable_if \
        <std::is_same<decltype(static_cast<end_t<Container>>(&Container::end))
            , end_t<Container>>::value>::type*);

    template <typename Container>
    static false_t& has(...);

public:
    static constexpr bool value = sizeof(has<T>(nullptr)) == sizeof(true_t);
};

// 支持非成员 std::begin / std::end 进行迭代的STL容器
template <class T>
struct support_free_begin_stl
    : public std::integral_constant<bool
        , has_const_iterator<T>::value
        && has_begin_iterator<T>::value
        && has_end_iterator<T>::value>
{
};

template <typename T>
struct support_free_begin_stl<std::valarray<T>>
    : public std::true_type
{
};

// stl containers
template <bool is_map, class Iterator, class charT>
inline void print_sequence(Iterator b, Iterator e
    , std::basic_ostream<charT>& out)
{
    // 只打印前 100 个元素
    constexpr size_t max_print_count = 100u;

    out << (is_map ? lbrace<charT>() : lbracket<charT>());
    for (size_t i = 0; b != e && i < max_print_count; ++i, ++b)
    {
        if (i > 0)
        {
            out << comma<charT>() << space<charT>();
        }
        out << *b;
    }
    if (b != e)
    {
        out << space<charT>() << ellipsis<charT>();
    }
    out << (is_map ? rbrace<charT>() : rbracket<charT>());
}

}  // namespace detail
}  // namespace tinylog

#if !defined(TINYLOG_DISABLE_STL_LOGING)

// @warn 加入 std 命名空间，有更好的方式?
namespace std
{

// std::pair
template<class charT, class Key, class Value>
inline std::basic_ostream<charT>&
operator<<(std::basic_ostream<charT>& out
    , std::pair<Key, Value> const& p)
{
    using namespace ::tinylog::detail;

    out << p.first
        << colon<charT>() << space<charT>()
        << p.second;
    return out;
}

// std::tuple
template<class charT, class... Args>
inline std::basic_ostream<charT>&
operator<<(std::basic_ostream<charT>& out
    , std::tuple<Args...> const& t)
{
    using namespace ::tinylog::detail;

    typename indices_generator<sizeof...(Args)>::type indices;
    print_tuple(t, indices, out);
    return out;
}

// stl::container
template <class charT, class Container>
inline typename std::enable_if
    <::tinylog::detail::support_free_begin_stl<Container>::value
    && !std::is_same<Container
        , std::basic_string<typename Container::value_type>>::value
    , std::basic_ostream<charT>&>::type
operator<<(std::basic_ostream<charT>& out
        , Container const& seq)
{
    using namespace ::tinylog::detail;

    print_sequence<has_kv<Container>::value>(std::begin(seq)
        , std::end(seq), out);
    return out;
}

}  // namesapce std

#endif  // TINYLOG_DISABLE_STL_LOGING

//////////////////////////////////////////////////////////////////////////////
//
// 十六进制倾印
//     输出格式 @see winhex
//
//////////////////////////////////////////////////////////////////////////////
namespace tinylog
{

template <bool hex_offset = false>
std::string hexdump(std::string const& data)
{
    using namespace std;

    constexpr auto off_len = 8;
    constexpr auto hex_size = 3;
    constexpr auto ascii_cnt  = 16;
    constexpr auto wide_len = off_len + hex_size * ascii_cnt + ascii_cnt + 2;

    constexpr auto space_char = ' ';
    constexpr auto sep_col = '|';
    constexpr auto sep_row = '-';

    string const off(hex_offset ? "HEX OFF" : "DEC OFF");
    string const ascii("ANSI ASCII");

    ostringstream oss;

    // title
    oss << setw((off_len - off.size()) / 2 + off.size()) << off
        << setw((off_len - off.size()) / 2) << space_char;
    oss << sep_col;
    for (auto i = 0; i != ascii_cnt; ++i)
    {
        oss << setw((hex_size - 1) / 2 + 1) << hex << uppercase << i
            << setw((hex_size - 1) / 2) << space_char;
    }
    oss << sep_col;
    oss << setw((ascii_cnt - ascii.size()) / 2 + ascii.size()) << ascii
        << setw((ascii_cnt - ascii.size()) / 2) << space_char << endl;
    oss << string(wide_len, sep_row) << endl;

    // content
    auto const row_size = data.size() / ascii_cnt
        + ((data.size() % ascii_cnt) ? 1 : 0);
    for (auto r = 0u; r != row_size; ++r)
    {
        auto const row_idx = r * ascii_cnt;
        oss << string((off_len - 9 > 0 ? off_len - 9 : 0), space_char)
            << setw(7) << setfill('0')
            << (hex_offset ? hex : dec) << uppercase << r * ascii_cnt
            << space_char;

        oss << sep_col;
        for (auto c = 0; c != ascii_cnt; ++c)
        {
            auto const idx = row_idx + c;
            auto const ch = data.size() > idx ? data[idx] : '\0';
            oss << setw(hex_size - 1) << hex << uppercase
                << (static_cast<unsigned>(ch) & 0xFF) << space_char;
        }
        oss << sep_col;

        for (auto c = 0; c != ascii_cnt; ++c)
        {
            auto const idx = row_idx + c;
            auto const ch = data.size() > idx ? data[idx] : space_char;
            oss << (isprint(ch) ? ch : space_char);
        }
        oss << endl;
    }
    return oss.str();
}

template <bool hex_offset = false>
inline std::string hexdump(char const* s, size_t n)
{
    return hexdump<hex_offset>(std::string(s, s + n));
}

template <bool hex_offset = false>
inline std::string hexdump(std::wstring const& s)
{
    char const* const p = (char const*)s.c_str();
    size_t const n = s.size() * sizeof(std::wstring::value_type);
    return hexdump<hex_offset>(p, n);
}

template <bool hex_offset = false>
inline std::string hexdump(wchar_t const* s, size_t n)
{
    return hexdump<hex_offset>(std::wstring(s, s + n));
}

template <bool hex_offset = false>
inline std::wstring whexdump(std::string const& s)
{
    auto const buf = hexdump<hex_offset>(s);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> u8_cvt;
    return u8_cvt.from_bytes(buf.data(), buf.data() + buf.size());
}

template <bool hex_offset = false>
inline std::wstring whexdump(char const* s, size_t n)
{
    return whexdump<hex_offset>(std::string(s, s + n));
}

template <bool hex_offset = false>
inline std::wstring whexdump(std::wstring const& s)
{
    char const* const p = (char const*)s.c_str();
    size_t const n = s.size() * sizeof(std::wstring::value_type);
    return whexdump<hex_offset>(p, n);
}

template <bool hex_offset = false>
inline std::wstring whexdump(wchar_t const* s, size_t n)
{
    return whexdump<hex_offset>(std::wstring(s, s + n));
}

}  // namespace tinylog

#endif  // TINYTINYLOG_HPP

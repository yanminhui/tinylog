/*
 _____ _             _
|_   _(_)_ __  _   _| |    ___   __ _
  | | | | '_ \| | | | |   / _ \ / _` | TinyLog for Modern C++
  | | | | | | | |_| | |__| (_) | (_| | version 1.1.7
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
 *     - 支持针对不同槽定制日志布局;
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
 * //     - [w]msvc_sink
 * //           visual studio debug console output.
 * //
 * logger::add_sink<sink::wfile_sink<default_layout>>("d:\\default.log");
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
 *
 * 1) 优化：重新调整代码结构, 支持针对不同槽定制布局v1.1.0 2018/05/27 yanmh
 * 2) 修正：wlout 拼写错误，layout using 错误. ----------- 2018/05/28 yanmh
 * 3) 修正：wcstombs 依赖全局 locale, std::cout 异常v1.1.1 2018/05/28 yanmh
 * 4) 优化：解耦终端颜色控制日志槽 v1.1.2 ---------------- 2018/06/01 yanmh
 * 5) 增强：支持 wchar_t/char 混合输出 v1.1.3 ------------ 2018/06/02 yanmh
 * 6) 增强: 模板参数可配互斥类型，支持槽过滤级别 v1.1.4 -- 2018/06/02 yanmh
 * 7) 优化: 日志时间精确到微秒 v1.1.5 -------------------- 2018/06/03 yanmh
 * 8) 增强: 支持条件日志 l[w]printf_if/[w]lout_if  v1.1.6  2018/06/03 yanmh
 * 9) 增强: 新增 msvc_sink(OutputDebugString(...)) v1.1.7  2018/06/03 yanmh
 * 10) 增强: 支持多个日志记录器 -------------------------- 2018/06/09 yanmh
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
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <valarray>
#include <vector>

// 版本信息
#define TINYLOG_VERSION_MAJOR 1
#define TINYLOG_VERSION_MINOR 1
#define TINYLOG_VERSION_PATCH 7

//--------------|
// 用户可控制   |
//--------------|

// 使用单线程模式
// #define TINYLOG_USE_SINGLE_THREAD 1

// 禁止STL容器日志
// #define TINYLOG_DISABLE_STL_LOGING 1

// 禁止终端输出颜色
// #define TINYLOG_DISABLE_CONSOLE_COLOR 1

// 使用简体中文
// #define TINYLOG_USE_SIMPLIFIED_CHINA 1

#if defined(TINYLOG_USE_SINGLE_THREAD)
#   define TINYLOG_NO_REGISTRY_MUTEX 1
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

#define TINYLOG_CRT_WIDE_(s) L ## s
#define TINYLOG_CRT_WIDE(s) TINYLOG_CRT_WIDE_(s)

// 间隔符
#define TINYLOG_SEPARATOR " "
#define TINYLOG_SEPARATORW TINYLOG_CRT_WIDE(TINYLOG_SEPARATOR)

#define TINYLOG_TITILE_CHAR '+'
#define TINYLOG_TITILE_CHARW TINYLOG_CRT_WIDE(TINYLOG_TITILE_CHAR)

#define TINYLOG_LF '\n'
#define TINYLOG_LFW TINYLOG_CRT_WIDE(TINYLOG_LF)

#define TINYLOG_DEFAULT "_TINYLOG_DEFAULT_"
#define TINYLOG_DEFAULTW TINYLOG_CRT_WIDE(TINYLOG_DEFAULT)

// 时间格式: 2018/05/20 19:30:27
#define TINYLOG_DATETIME_FMT "%Y/%m/%d %H:%M:%S."
#define TINYLOG_DATETIME_FMTW TINYLOG_CRT_WIDE(TINYLOG_DATETIME_FMT)

#if defined(TINYLOG_USE_SIMPLIFIED_CHINA)

// 调试打印格式: 文件 main.cpp, 行 24, 函数 main()
#   define TINYLOG_DEBUG_PRINTF_FMT     "文件 %s, 行 %u, 函数 %s\n"
#   define TINYLOG_DEBUG_PRINTF_FMTW    L"文件 %ls, 行 %u, 函数 %ls\n"

// 日志级别
#   define TINYLOG_LEVEL_TRACE  "跟踪"
#   define TINYLOG_LEVEL_DEBUG  "调试"
#   define TINYLOG_LEVEL_INFO   "信息"
#   define TINYLOG_LEVEL_WARN   "警告"
#   define TINYLOG_LEVEL_ERROR  "错误"
#   define TINYLOG_LEVEL_FATAL  "严重"
#   define TINYLOG_LEVEL_UNKOWN "未知"

#else

// 调试打印格式: 文件 main.cpp, 行 24, 函数 main()
#   define TINYLOG_DEBUG_PRINTF_FMT     "file %s, line %u, func %s\n"
#   define TINYLOG_DEBUG_PRINTF_FMTW    L"file %ls, line %u, func %ls\n"

// 日志级别
#   define TINYLOG_LEVEL_TRACE  "TRACE"
#   define TINYLOG_LEVEL_DEBUG  "DEBUG"
#   define TINYLOG_LEVEL_INFO   "INFO"
#   define TINYLOG_LEVEL_WARN   "WARN"
#   define TINYLOG_LEVEL_ERROR  "ERROR"
#   define TINYLOG_LEVEL_FATAL  "FATAL"
#   define TINYLOG_LEVEL_UNKOWN "UNKNOWN"

#endif // TINYLOG_USE_SIMPLIFIED_CHINA

#define TINYLOG_LEVEL_TRACEW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_TRACE)
#define TINYLOG_LEVEL_DEBUGW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_DEBUG)
#define TINYLOG_LEVEL_INFOW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_INFO)
#define TINYLOG_LEVEL_WARNW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_WARN)
#define TINYLOG_LEVEL_ERRORW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_ERROR)
#define TINYLOG_LEVEL_FATALW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_FATAL)
#define TINYLOG_LEVEL_UNKOWNW TINYLOG_CRT_WIDE(TINYLOG_LEVEL_UNKOWN)

#if defined(__GNUC__)
#   define TINYLOG_FUNCTION __PRETTY_FUNCTION__
#else
#   define TINYLOG_FUNCTION __FUNCTION__
#endif // __PRETTY_FUNCTION__

#if defined(NDEBUG)

#   define dlprintf(ln, lvl, fmt, ...) \
    for (::tinylog::detail::dlprintf_impl _tl_strm_((ln), (lvl)) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlwprintf(ln, lvl, fmt, ...) \
    for (::tinylog::detail::dlwprintf_impl _tl_strm_((ln), (lvl)) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlout(ln, lvl) \
    for (::tinylog::detail::odlstream _tl_strm_((ln), (lvl)) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#   define wdlout(ln, lvl) \
    for (::tinylog::detail::wodlstream _tl_strm_((ln), (lvl)) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#else

#   define dlprintf(ln, lvl, fmt, ...) \
    for (::tinylog::detail::dlprintf_d_impl _tl_strm_((ln), (lvl) \
            , __FILE__, __LINE__, TINYLOG_FUNCTION) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlwprintf(ln, lvl, fmt, ...) \
    for (::tinylog::detail::dlwprintf_d_impl _tl_strm_((ln), (lvl) \
            , TINYLOG_CRT_WIDE(__FILE__), __LINE__ \
            , ::tinylog::a2w(TINYLOG_FUNCTION)) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_((fmt), ##__VA_ARGS__)

#   define dlout(ln, lvl) \
    for (::tinylog::detail::odlstream_d _tl_strm_((ln), (lvl) \
            , __FILE__, __LINE__, TINYLOG_FUNCTION) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#   define wdlout(ln, lvl) \
    for (::tinylog::detail::wodlstream_d _tl_strm_((ln), (lvl) \
            , TINYLOG_CRT_WIDE(__FILE__), __LINE__ \
            , ::tinylog::a2w(TINYLOG_FUNCTION)) \
         ; _tl_strm_; _tl_strm_.flush()) _tl_strm_

#endif  // NDEBUG

// 默认日志记录器
#define lprintf(lvl, fmt, ...) \
    dlprintf(TINYLOG_DEFAULT, (lvl), (fmt), ##__VA_ARGS__)
#define lwprintf(lvl, fmt, ...) \
    dlwprintf(TINYLOG_DEFAULTW, (lvl), (fmt), ##__VA_ARGS__)
#define lout(lvl) \
    dlout(TINYLOG_DEFAULT, (lvl))
#define wlout(lvl) \
    wdlout(TINYLOG_DEFAULTW, (lvl))

// 条件日志
#define dlprintf_if(ln, lvl, boolexpr, fmt, ...) \
    if ((boolexpr)) dlprintf((ln), (lvl), (fmt), ##__VA_ARGS__)
#define dlwprintf_if(ln, lvl, boolexpr, fmt, ...) \
    if ((boolexpr)) dlwprintf((ln), (lvl), (fmt), ##__VA_ARGS__)
#define dlout_if(ln, lvl, boolexpr) \
    if ((boolexpr)) dlout((ln), (lvl))
#define wdlout_if(ln, lvl, boolexpr) \
    if ((boolexpr)) wdlout((ln), (lvl))

// 条件日志：默认日志记录器
#define lprintf_if(lvl, boolexpr, fmt, ...) \
    if ((boolexpr)) lprintf((lvl), (fmt), ##__VA_ARGS__)
#define lwprintf_if(lvl, boolexpr, fmt, ...) \
    if ((boolexpr)) lwprintf((lvl), (fmt), ##__VA_ARGS__)
#define lout_if(lvl, boolexpr) \
    if ((boolexpr)) lout((lvl))
#define wlout_if(lvl, boolexpr) \
    if ((boolexpr)) wlout((lvl))

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

namespace tinylog
{
//////////////////////////////////////////////////////////////////////////////
//
// 辅助函数
//
//////////////////////////////////////////////////////////////////////////////
namespace detail
{
//
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
    bool try_lock()
    {
        return true;
    }
};

// get current time
struct time_value
{
    std::time_t tv_sec;
    std::size_t tv_usec;
};

inline time_value curr_time()
{
    using namespace std::chrono;

    auto const tp   = system_clock::now();
    auto const dtn  = tp.time_since_epoch();
    auto const sec  = duration_cast<seconds>(dtn).count();
    auto const usec = duration_cast<microseconds>(dtn).count() % 1000000;
    return { sec, static_cast<std::size_t>(usec) };
}

// get current thread id
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

// 实现 strftime 格式化当前时间
// 返回 std::basic_string<charT>
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

// 确保 l[w]printf 可变参数有效
//
// 如，可防止以下错误:
//      std::string s("hello");
//      std::printf("%s", s); // printf 不支持 std::string，但又不报错。
//
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

//
// s[w]printf 实现: 将可变参数格式化成字符串.
//
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
#if !defined(NDEBUG)
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
#if !defined(NDEBUG)
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

// 获取文件大小
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

} // namespace detail

//////////////////////////////////////////////////////////////////////////////
//
// 编码转换
//
//////////////////////////////////////////////////////////////////////////////

//
// 不同类型编码字符串转换
//
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

//
// 特定编码字符串构建
//
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
        // perhaps too large, but that's OK
        // encodings like shift-JIS need some prefix space
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
        // perhaps too large, but that's OK
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

//
// implement
//
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

//////////////////////////////////////////////////////////////////////////////
//
// 日志级别
//
//////////////////////////////////////////////////////////////////////////////

enum level : std::uint8_t
{
    trace, debug, info, warn, error, fatal
};

template <class charT>
std::basic_string<charT> to_string(level lvl);

template <>
std::basic_string<char> to_string(level lvl)
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
std::basic_string<wchar_t> to_string(level lvl)
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

//////////////////////////////////////////////////////////////////////////////
//
// 记录结构
//
//////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////
//
// 构建布局：
//     序列化记录结构
//
//////////////////////////////////////////////////////////////////////////////
namespace detail
{

template <class deriveT, class charT>
struct layout_constructor_base
{
    using derive    = deriveT;
    using char_type = charT;
    using string_t  = std::basic_string<char_type>;
    using record    = basic_record<char_type>;
    using record_d  = basic_record_d<char_type>;

    static void construct(string_t& s, record const& r)
    {
        derive::gen_record_msg(s, r);
    }

    static void construct(string_t& s, record_d const& r)
    {
        derive::gen_record_msg(s, r);
        derive::append_debug_msg(s, r);
    }

protected:
    // time [level] #id message
    static void gen_record_msg(string_t& s, record const& p);

    // file line func
    static void append_debug_msg(string_t& s, record_d const& p);

protected:
    static void end_with(string_t& s, char_type delimiter)
    {
        if (s.empty())
        {
            s.push_back(delimiter);
            return ;
        }
        if (*s.crbegin() != delimiter)
        {
            s.push_back(delimiter);
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

    using base      = layout_constructor_base<layout_constructor<char>, char>;

    static constexpr auto sep = TINYLOG_SEPARATOR;
    static constexpr auto lf  = TINYLOG_LF;

protected:
    // time [level] #id message
    static void gen_record_msg(string_t& s, record const& r)
    {
        s = format_time(r) + sep + "[" + to_string<char_type>(r.lvl) + "]"
            + sep + "#" + std::to_string(r.id)
            + sep + r.message;
        end_with(s, lf);
    }

    // file line func
    static void append_debug_msg(string_t& s, record_d const& r)
    {
        string_t debug_info;
        sprintf_constructor<char_type>::construct(debug_info
                , TINYLOG_DEBUG_PRINTF_FMT
                , r.file.c_str(), r.line, r.func.c_str());
        end_with(debug_info, lf);
        s += debug_info;
    }

private:
    static string_t format_time(record const& r)
    {
        return strftime_impl(std::strftime, TINYLOG_DATETIME_FMT, r.tv);
    }
};

template <>
struct layout_constructor<wchar_t>
    : public layout_constructor_base<layout_constructor<wchar_t>, wchar_t>
{
    friend struct layout_constructor_base<layout_constructor<wchar_t>, wchar_t>;

    using base = layout_constructor_base<layout_constructor<wchar_t>, wchar_t>;

    static constexpr auto sep = TINYLOG_SEPARATORW;
    static constexpr auto lf  = TINYLOG_LFW;

protected:
    // time [level] #id message
    static void gen_record_msg(string_t& s, record const& r)
    {
        s = format_time(r) + sep + L"[" + to_string<char_type>(r.lvl) + L"]"
            + sep + L"#" + std::to_wstring(r.id)
            + sep + r.message;
        end_with(s, lf);
    }

    // file line func
    static void append_debug_msg(string_t& s, record_d const& r)
    {
        string_t debug_info;
        sprintf_constructor<char_type>::construct(debug_info
                , TINYLOG_DEBUG_PRINTF_FMTW
                , r.file.c_str(), r.line, r.func.c_str());
        end_with(debug_info, lf);
        s += debug_info;
    }

private:
    static string_t format_time(record const& r)
    {
        return strftime_impl(std::wcsftime, TINYLOG_DATETIME_FMTW, r.tv);
    }
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////////////
//
// 格式化器：
//     构建布局, 将布局生成消息.
//
//////////////////////////////////////////////////////////////////////////////

//
// 默认布局: 用户可定制, 实现不同布局.
//
// @note this is default layout.
//
struct default_layout
{
    template <class stringT, class recordT>
    static void to_string(stringT& s, recordT const& r)
    {
        using char_type = typename stringT::value_type;
        detail::layout_constructor<char_type>::construct(s, r);
    }
};

//
// 格式化工具: 提供格式化方法，利用布局将记录序列化.
//
template <class layoutT = default_layout>
struct formatter
{
    template <class recordT>
    static typename recordT::string_t format(recordT const& r)
    {
        typename recordT::string_t s;
        layoutT::to_string(s, r);
        return s;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// 终端颜色样式
//
//////////////////////////////////////////////////////////////////////////////
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
    emphasize em;
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
            oss << oss.widen('\033') << oss.widen('[') << n << oss.widen('m');
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

//////////////////////////////////////////////////////////////////////////////
//
// 日志输出槽:
//     使用者可以定制，实现属于自己的槽，使日志消息输出到不同对象上。
//
//////////////////////////////////////////////////////////////////////////////

// 互斥量类型
#if defined(TINYLOG_USE_SINGLE_THREAD)
using mutex_t = detail::null_mutex;
#else
using mutex_t = std::mutex;
#endif

namespace sink
{
//
// 抽象基类:
//     使用者可以继承 basic_sink<> 实现，实现属于自己的槽。
//
// @attention 类中的纯虚函数一定要实现。
//
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

    // 是否打开准备好接收数据
    virtual bool is_open() const = 0;

    // 消费数据
    virtual void consume(basic_record<char_type> const& r) = 0;
    virtual void consume(basic_record_d<char_type> const& r) = 0;

private:
    level lvl_ = level::trace;
};

template <class charT, class layoutT = default_layout
    , class mutexT = mutex_t
    , class formatterT = formatter<layoutT>>
class basic_sink : public basic_sink_base<charT>
{
public:
    using base      = basic_sink_base<charT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

    void consume(basic_record<char_type> const& r) override final
    {
        auto msg = formatterT::format(r);
        write(r.lvl, msg);
    }

    void consume(basic_record_d<char_type> const& r) override final
    {
        auto msg = formatterT::format(r);
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
        {   // 保护中...
            std::lock_guard<mutexT> lock(mtx_);

            before_writing(lvl, msg);
            writing(lvl, msg);
            after_writing(lvl, msg);
        }
        after_write(lvl, msg);
    }

protected:
    // 在输出数据之前可能需要对数据进行转换处理
    virtual void before_write(level lvl, string_t& msg)
    {
    }

    // 消费日志消息：before_writing / writing / after_writing 互斥中
    //     不要在这里做太多耗时性作业
    //     尽量把工作转移到 before_write / after_write
    virtual void before_writing(level lvl, string_t& msg)
    {
    }

    virtual void writing(level lvl, string_t& msg) = 0;

    virtual void after_writing(level lvl, string_t& msg)
    {
    }

    // 在输出数据之后做一些遗后工作
    virtual void after_write(level lvl, string_t& msg)
    {
    }

private:
    mutexT mtx_;
};

//----------------|
// Console Sink   |
//----------------|

template <class charT, class layoutT = default_layout
    , class mutexT = mutex_t
    , class formatterT = formatter<layoutT>>
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

    void enable_color(bool enable = true)
    {
        enable_color_ = enable;
    }

protected:
    void writing(level lvl, string_t& msg) override final
    {
        std::basic_istringstream<char_type> iss(msg);
        string_t line;
        while (std::getline(iss, line))
        {
            write_line(lvl, line);
        }
    }

private:
    template <class lineCharT, typename std::enable_if
        <std::is_same<lineCharT, char>::value, int>::type = 0>
    void write_line(level lvl, std::basic_string<lineCharT> const& line) const
    {
        if (!enable_color_)
        {
            std::printf("%s\n", line.c_str());
            return ;
        }

        std::printf("%s", style_beg(lvl).c_str());
        std::printf("%s", line.c_str());
        std::printf("%s", style_end().c_str());
        std::printf("\n");
    }

    template <class lineCharT, typename std::enable_if
        <std::is_same<lineCharT, wchar_t>::value, int>::type = 0>
    void write_line(level lvl, std::basic_string<lineCharT> const& line) const
    {
        if (!enable_color_)
        {
            std::wprintf(L"%ls\n", line.c_str());
            return ;
        }

        std::wprintf(L"%ls", style_beg(lvl).c_str());
        std::wprintf(L"%ls", line.c_str());
        std::wprintf(L"%ls", style_end().c_str());
        std::wprintf(L"\n");
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
#if defined(TINYLOG_DISABLE_CONSOLE_COLOR)
    bool enable_color_ = false;
#else
    bool enable_color_ = true;
#endif
};

using console_sink  = basic_console_sink<char>;
using wconsole_sink = basic_console_sink<wchar_t>;

//----------------|
// File Sink      |
//----------------|

template <class charT, class layoutT = default_layout
    , class mutexT = mutex_t
    , class formatterT = formatter<layoutT>>
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
    void before_writing(level lvl, string_t& msg) override final
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
            /* 吞下苦果，避免应用异常退出。*/
        }
        ostrm_.clear();
        ostrm_.open(filename_, std::ios_base::out);
    }

    void writing(level lvl, string_t& msg) override final
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
    , class formatterT = formatter<layoutT>>
class basic_u8_file_sink
    : public basic_file_sink<charT, layoutT, mutexT, formatterT>
{
public:
    using base      = basic_file_sink<charT, layoutT, mutexT, formatterT>;
    using char_type = typename base::char_type;
    using string_t  = typename base::string_t;

public:
    explicit basic_u8_file_sink
    (char const* filename
     , std::uintmax_t max_file_size
     = base::default_max_file_size
       , std::ios_base::openmode mode
     = std::ios_base::app
       , std::locale const& loc
     = std::locale(std::locale(""), new std::codecvt_utf8<wchar_t>))
        : base(filename, max_file_size, mode, loc)
    {
    }

protected:
    void before_write(level lvl, string_t& msg) override final
    {
        // convert only if string_t is narrow type.
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
    , class formatterT = formatter<layoutT>>
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

//////////////////////////////////////////////////////////////////////////////
//
// 槽适配器
//
//////////////////////////////////////////////////////////////////////////////
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
    using char_type     = typename std::conditional<std::is_same
        <charT, char>::value, char, wchar_t>::type;
    using extern_type   = typename std::conditional<!std::is_same
        <charT, char>::value, char, wchar_t>::type;

    using string_t  = std::basic_string<char_type>;
    using sink_t    = std::shared_ptr<sink::basic_sink_base<char_type>>;

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

//////////////////////////////////////////////////////////////////////////////
//
// 日志记录器
//
//////////////////////////////////////////////////////////////////////////////

class logger
{
public:
#if defined(TINYLOG_WINDOWS_API)
    using char_type     = wchar_t;
    using extern_type   = char;
#else
    using char_type     = char;
    using extern_type   = wchar_t;
#endif // defined
    using string_t      = std::basic_string<char_type>;
    using xstring_t     = std::basic_string<extern_type>;
    using sink_adapter_t = std::shared_ptr<detail::sink_adapter_base>;

public:
    explicit logger(string_t const& n) : name_(n)
    {
    }
    explicit logger(xstring_t const& n)
    {
        string_traits<>::convert(name_, n);
    }
    explicit logger() :
#if defined(TINYLOG_WINDOWS_API)
        name_(TINYLOG_DEFAULTW)
#else
        name_(TINYLOG_DEFAULT)
#endif
    {
    }

    string_t name() const
    {
        return name_;
    }

    /** \brief 过滤日志级别
     *
     * 低于 lvl 级别的日志不输出
     * 优先级:
     *     trace < debug < info < warn < error < fatal
     */
    void set_level(level lvl)
    {
        lvl_ = lvl;
    }

    level get_level() const
    {
        return lvl_;
    }

    /** \brief 安装输出槽
     *
     * 使用形式参见 std::make_shared(...)
     *
     * 示例:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.cpp
     * auto file_sink = logger::create_sink<sink::wfile_sink>(L"d:\\error.log");
     * if (!*file_sink) {
     *     // open file failed.
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    template <class sinkT, class... Args>
    std::shared_ptr<sinkT> create_sink(Args&&... args)
    {
        auto sk = std::make_shared<sinkT>(std::forward<Args>(args)...);
        return add_sink(sk);
    }

    template <class sinkT
            , class charT = typename sinkT::char_type>
    std::shared_ptr<sinkT> add_sink(std::shared_ptr<sinkT> sk)
    {
        using char_type = charT;
        assert(sk && "sink instance point must exists");
        sk->set_level(lvl_);
        auto sk_adapter = std::make_shared<detail::basic_sink_adapter<char_type>>(sk);
        sink_adapters_.emplace_back(sk_adapter);
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
                // TODO: failed
                continue;
            }
            sk_adapter->consume(r);
        }
    }

public:
    /** \brief 产生一个标题文本
     *
     * 一般在对日志设置完毕后，做为第一条日志记录，
     * 让日志文件看起来有明显的边界。
     */
    static std::string title(std::string const& text = "TinyLog")
    {
        return detail::gen_title<std::string::value_type>(text
                , TINYLOG_TITILE_CHAR);
    }

    static std::wstring wtitle(std::wstring const& text = L"TinyLog")
    {
        return detail::gen_title(text, TINYLOG_TITILE_CHARW);
    }

private:
    string_t name_;
    level lvl_ = level::trace;
    std::vector<sink_adapter_t> sink_adapters_;
};

//
// 注册管理
//     管理日志记录器
//
template<class mutexT = mutex_t>
class basic_registry
{
public:
#if defined(TINYLOG_WINDOWS_API)
    using char_type     = wchar_t;
    using extern_type   = char;
#else
    using char_type     = char;
    using extern_type   = wchar_t;
#endif // defined
    using string_t      = std::basic_string<char_type>;
    using xstring_t     = std::basic_string<extern_type>;
    using logger_t      = logger;
    using logger_ptr    = std::shared_ptr<logger_t>;

public:
    static basic_registry& instance()
    {
        if (!inst_)
        {
            struct ms_enabler : public basic_registry {};

            if (std::this_thread::get_id() == std::thread::id())
            // on main
            {
                inst_ = std::make_shared<ms_enabler>();
            }
            else
            {
                std::call_once(once_flg_
                    , [&]()
                    {
                        inst_ = std::make_shared<ms_enabler>();
                    });
            }
        }
        assert(inst_ && "registry instance must exists");
        return *inst_;
    }

public:
    basic_registry(basic_registry const&) = delete;
    basic_registry& operator=(basic_registry const&) = delete;

    // 日志级别:
    //     为创建日志记录器时设置默认过滤级别
    void set_level(level lvl)
    {
        std::lock_guard<mutexT> lock(mtx_);
        for (auto& l : loggers_)
        {
            l.second->set_level(lvl);
        }
        lvl_ = lvl;
    }

    // 添加日志记录器
    //      失败将抛出异常
    logger_ptr create_logger(string_t const& logger_name)
    {
        std::lock_guard<mutexT> lock(mtx_);
        throw_if_exists(logger_name);
        auto p = std::make_shared<logger_t>(logger_name);
        p->set_level(lvl_);
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
        std::lock_guard<mutexT> lock(mtx_);
        throw_if_exists(name);
        inst->set_level(lvl_);
        loggers_[name] = inst;
        return inst;
    }

    // 获取日志记录器
    //      失败返回空指针
    logger_ptr get_logger(string_t const& logger_name) const
    {
        std::lock_guard<mutexT> lock(mtx_);
        auto found = loggers_.find(logger_name);
        return found == loggers_.cend() ? nullptr : found->second;
    }
    logger_ptr get_logger(xstring_t const& logger_name) const
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return get_logger(name);
    }
    logger_ptr get_logger() const
    {
#if defined(TINYLOG_WINDOWS_API)
        return get_logger(TINYLOG_DEFAULTW);
#else
        return get_logger(TINYLOG_DEFAULT);
#endif
    }

    void erase_logger(string_t const& logger_name)
    {
        std::lock_guard<mutexT> lock(mtx_);
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

    void earse_all_logger()
    {
        std::lock_guard<mutexT> lock(mtx_);
        loggers_.clear();
    }

private:
    basic_registry() = default;

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
            name = "logger with name \'" + name + "\' already exists";
            throw std::system_error(std::make_error_code(std::errc::file_exists)
                                    , name);
        }
    }

private:
    static std::once_flag once_flg_;
    static std::shared_ptr<basic_registry> inst_;

private:
    mutable mutexT mtx_;
    level lvl_ = level::trace;
    std::unordered_map<string_t, logger_ptr> loggers_;
};

template <class mutexT>
std::once_flag basic_registry<mutexT>::once_flg_;

template <class mutexT>
std::shared_ptr<basic_registry<mutexT>> basic_registry<mutexT>::inst_;

#if defined(TINYLOG_NO_REGISTRY_MUTEX)
using registry = basic_registry<detail::null_mutex>;
#else
using registry = basic_registry<std::mutex>;
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  捕获日志记录
//
//////////////////////////////////////////////////////////////////////////////
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

    explicit basic_dlprintf_base(string_t const& logger_name)
        : logger_(registry::instance().get_logger(logger_name))
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
        : base(logger_name), record_(lvl)
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
        sprintf_constructor<char_type>::construct(record_.message
                , fmt.c_str(), std::forward<Args>(args)...);
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
        : base(logger_name), record_(lvl, file, line, func)
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
        sprintf_constructor<char_type>::construct(record_.message
                , fmt.c_str(), std::forward<Args>(args)...);
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

// 流化支持: 日志输出流
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

    explicit basic_odlstream_base(string_t const& logger_name)
        : base(&stringbuf_)
        , logger_(registry::instance().get_logger(logger_name))
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
        : base(logger_name), record_(lvl)
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

// 流化支持: 支持调试信息
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
        : base(logger_name)
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

template <class charT>
struct delimiters;

template <>
struct delimiters<char>
{
    static constexpr auto ellipsis      = "...";
    static constexpr auto space         = ' ';
    static constexpr auto comma         = ',';
    static constexpr auto colon         = ':';
    static constexpr auto lparenthese   = '(';
    static constexpr auto rparenthese   = ')';
    static constexpr auto lbracket      = '[';
    static constexpr auto rbracket      = ']';
    static constexpr auto lbrace        = '{';
    static constexpr auto rbrace        = '}';
};

template <>
struct delimiters<wchar_t>
{
    static constexpr auto ellipsis      = L"...";
    static constexpr auto space         = L' ';
    static constexpr auto comma         = L',';
    static constexpr auto colon         = L':';
    static constexpr auto lparenthese   = L'(';
    static constexpr auto rparenthese   = L')';
    static constexpr auto lbracket      = L'[';
    static constexpr auto rbracket      = L']';
    static constexpr auto lbrace        = L'{';
    static constexpr auto rbrace        = L'}';
};

// std::tuple
template<class Tuple, size_t... Indices
         , class charT, class delimiterT = delimiters<charT>>
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
        return { delimiterT::comma, delimiterT::space };
    };

    out << delimiterT::lparenthese;
    (void)swallow{0, (out << delimiters(Indices) << std::get<Indices>(t), 0)...};
    out << delimiterT::rparenthese;
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
    static true_t& has(typename std::enable_if<std::is_same
                       <decltype(static_cast<begin_t<Container>>
                                 (&Container::begin))
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
    static true_t& has(typename std::enable_if<std::is_same
                       <decltype(static_cast<end_t<Container>>
                                 (&Container::end))
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
template <bool is_map, class Iterator
          , class charT, class delimiterT = delimiters<charT>>
inline void print_sequence(Iterator b, Iterator e
                           , std::basic_ostream<charT>& out)
{
    // 只打印前 100 个元素
    constexpr size_t max_print_count = 100u;

    out << (is_map ? delimiterT::lbrace : delimiterT::lbracket);
    for (size_t i = 0; b != e && i < max_print_count; ++i, ++b)
    {
        if (i > 0)
        {
            out << delimiterT::comma << delimiterT::space;
        }
        out << *b;
    }
    if (b != e)
    {
        out << delimiterT::space << delimiterT::ellipsis;
    }
    out << (is_map ? delimiterT::rbrace : delimiterT::rbracket);
}

}  // namespace detail
}  // namespace tinylog

#if !defined(TINYLOG_DISABLE_STL_LOGING)

// @warn 加入 std 命名空间，有更好的方式?
namespace std
{

// std::pair
template<class charT, class Key, class Value
         , class delimiterT = ::tinylog::detail::delimiters<charT>>
inline std::basic_ostream<charT>&
operator<<(std::basic_ostream<charT>& out
           , std::pair<Key, Value> const& p)
{
    using namespace ::tinylog::detail;

    out << p.first
        << delimiterT::colon << delimiterT::space
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
operator<<(std::basic_ostream<charT>& out, Container const& seq)
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
    auto buf = hexdump<hex_offset>(s);
    std::wstring ws;
    string_traits<u8string>::convert(ws, buf);
    return ws;
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

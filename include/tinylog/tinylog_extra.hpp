/* Support for print STL container and string in hex.
 *
 * ```.cpp
 *
 * using namespace tinylog;
 *
 * int main()
 * {
 *     // Print STL container.
 *     std::vector<int> v = { 1, 2, 3, 4, 5 };
 *     std::cout << v << std::endl;
 *
 *     // Print string in hex.
 *     std::cout << hexdump("Bravo! The job has been done well.") << std::endl;
 * }
 *
 * ```
 */

/*****************************************************************************/
/* User Customize:
 *
 *   *** Tinylog basic setting see 'tinylog.hpp' ***
 */

// Tinylog can print stl container to be similar to what python's print
// function did. If you choose disable the feature, just remove the
// double-slash.
//
// For example:
//     pair  --> key: value
//     tuple --> (item_0, item_1, item_2)
//     list  --> [item_0, item_1, item_2 ]
//     map   --> { key_0: value_0, key_1: value_1 }

// #define TINYLOG_DISABLE_STL_LOGGING 1


// --- User Customize End ---

#ifndef TINYTINYLOG_EXTRA_HPP
#define TINYTINYLOG_EXTRA_HPP

#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <valarray>
#include <vector>

namespace tinylog
{
namespace detail
{
// c++14 make_index_sequence
//
// @see https://github.com/hokein/Wiki/wiki/
//      How to unpack a std::tuple to a function with multiple arguments?
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

/*****************************************************************************/
/* Output STL Coontainer Support. */

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
    (void)swallow{0
        , (out << delimiters(Indices) << std::get<Indices>(t), 0)...};
    out << delimiterT::rparenthese;
}

struct sfinae
{
    using true_t = char;
    using false_t = char[2];
};

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

// std::map
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

// begin()
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

// end()
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

// STL containers
template <bool is_map, class Iterator
          , class charT, class delimiterT = delimiters<charT>>
inline void print_sequence(Iterator b, Iterator e
                           , std::basic_ostream<charT>& out)
{
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

} // namespace detail
} // namespace tinylog

#if !defined(TINYLOG_DISABLE_STL_LOGGING)

// @warn Override in std namespace. Better method ?
namespace std
{

// std::pair
template<class charT, class Key, class Value
         , class delimiterT = ::tinylog::detail::delimiters<charT>>
inline std::basic_ostream<charT>&
operator<<(std::basic_ostream<charT>& out, std::pair<Key, Value> const& p)
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
operator<<(std::basic_ostream<charT>& out, std::tuple<Args...> const& t)
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

} // namesapce std

#endif  // TINYLOG_DISABLE_STL_LOGGING

/*****************************************************************************/
/* Dump String in Hex: @see winhex. */

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

#endif  // TINYTINYLOG_EXTRA_HPP

# TinyLog

[![GitHub Author](https://img.shields.io/badge/author-%E9%A2%9C%E9%97%BD%E8%BE%89-blue.svg)](mailto:yanminhui163@163.com)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://www.github.com/yanminhui/tinylog/tree/master/LICENSE)
[![GitHub Releases](https://img.shields.io/github/release/yanminhui/tinylog.svg)](https://github.com/yanminhui/tinylog/releases)
[![GitHub Platform](https://img.shields.io/badge/platform-%20linux%20%7C%20macos%20%7C%20windows%20-brightgreen.svg)](https://github.com/yanminhui/tinylog/tree/master/README.md)
[![GitHub Issues](https://img.shields.io/github/issues/yanminhui/tinylog.svg)](https://github.com/yanminhui/tinylog/issues)

[**English**](https://github.com/yanminhui/tinylog/tree/master/README.md)    [**简体中文**](https://github.com/yanminhui/tinylog/tree/master/README_CN.md)

## Design goals

Thare are myriads of logging libraries of C++ out there. Many of them have extensive functionality，the often complex configuration, and provide interfaces in a shared library. Our class had these design goals:

- **Trivial integration**： Our whole code consistes of a single header file [`tinylog.hpp`](https://github.com/yanminhui/tinylog/tree/master/include/tinylog.hpp). That's it. No library, no subject, no dependencies, no complex build system.

- **Custom output sink**：Customizing ouput sink by inheriting from the class `basic_sink`. Console color sink and file sink is avaliable by default.

- **Custom layout**: You can specify your own layout specifiers. In order to do that you can use template parameters in output sinks.

- **Formatting**：Logging like `printf` is available. This is done by using `l[w]printf` marcos. Logging like `std::cout` is recommanded.

- **STL logging**：You can log your STL template including most containers，and containers may be supported in other libraries.

- **Memory dump**：Dump bytes stream by using `hexdump`.

- **Internationalized**：`wchar_t` and `char` string output is supported.

- **Thread and type safe**：By default thread-safety is enabled, you can disbled it  by defining `TINYLOG_USE_SINGLE_THREAD`.

## Supported compilers

Currently, the following compilers are known to work:

- GCC 5.4.0 / Ubuntu 16.04.3 LTS
- Clang 9.1.0 / MacOS 10.13.4
- Microsoft Visual C++ 2015 / Windows 7

## Integration

[`tinylog.hpp`](https://github.com/yanminhui/tinylog/tree/master/include/tinylog.hpp) is the single required file in [`include`](https://github.com/yanminhui/tinylog/tree/master/include). You need to add

~~~cpp
#include "tinylog.hpp"

// use namespace tinylog
using namespace tinylog;
~~~

to the files you want to logging and set the necessary switches to enable C++11 (e.g., -std=c++11 for GCC and Clang).

## Example

~~~cpp
#include "tinylog.hpp"

#include <locale>
#include <map>
#include <string>

int main(int argc, char* argv[])
{
    using namespace tinylog;

    //--------------|
    // Setting      |
    //--------------|
    std::locale loc("");
    std::locale::global(loc);

    // Setup sink: @see std::make_shared<>
    logger::add_sink<sink::console_sink>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    logger::add_sink<sink::u8_file_sink>("default.log", max_file_size);

    // Filter level
    logger::set_level(debug);

    // [option] Output title
    lout_d << logger::title("TinyLog");

    //--------------|
    // Logging      |
    //--------------|
    // Normal text
    lout(info) << "Weclome to TinyLog !!!" << std::endl;

    // STL container
    std::map<std::string, size_t> const ages = {{ "tinylog", 1 }, { "json", 5 }};
    lout(warn) << "ages: " << ages << std::endl;

    // Hex string
    constexpr auto text = "Bravo! The job has been done well.";
    lout(error) << "hexdump: " << text << "\n" << hexdump(text);

    return 0;
}
~~~

> narrow version see [example_narrow](https://github.com/yanminhui/tinylog/tree/master/example/example_narrow.cpp)
> unicode version see [example_unicode](https://github.com/yanminhui/tinylog/tree/master/example/example_unicode.cpp)

![example_narrow](https://raw.githubusercontent.com/yanminhui/tinylog/master/example/example_narrow.png)

## License

<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

The class is licensed under the [MIT License](http://opensource.org/licenses/MIT):

Copyright &copy; 2018 [颜闽辉](mailto:yanminhui163@163.com)

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

## Contact

If you have questions regarding the library，I would like to invite you to [open an issue at GitHub](https://github.com/yanminhui/tinylog/issues/new)，Opening an issue at GitHub allows other users and contributors to this library to collaborate.

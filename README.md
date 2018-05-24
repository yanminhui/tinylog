# TinyLog

[![GitHub Author](https://img.shields.io/badge/%E4%BD%9C%E8%80%85-%E9%A2%9C%E9%97%BD%E8%BE%89-blue.svg)](mailto:yanminhui163@163.com)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://www.github.com/yanminhui/tinylog/tree/master/LICENSE)
[![GitHub Releases](https://img.shields.io/github/release/yanminhui/tinylog.svg)](https://github.com/yanminhui/tinylog/releases)
[![GitHub Platform](https://img.shields.io/badge/platform-linux%2Fmacos%2Fwindows-brightgreen.svg)](https://github.com/yanminhui/tinylog/tree/master/README.md)
[![GitHub Issues](https://img.shields.io/github/issues/yanminhui/tinylog.svg)](http://github.com/yanminhui/tinylog/issues)

## 设计目标

当前存在不少 C++ 的日志工具库，很多都功能丰富，配置也较为繁重，以共享库的方式提供。工具最主要的目标是： **满足条件，易于使用。** 我们使用现代 C++ 编程能力，旨在实现一个尽可能满足条件的日志工具库。

- **易于整合**： 只需要一个头文件，并且没有任何依赖。

- **定制输出槽**：可自定义日志输出槽，将记录输出到不同目标。默认提供了日志级别颜色显示的终端输出，以及支持 UTF-8 编码的文件输出。

- **格式化**：同时支持 `l[w]printf` 格式化语法，以及 `[w]lout` 流输出。并且增强对 `printf` 格式化语法参数合法性的检查。

- **STL日志**：支持 STL 容器对象格式化输出。

- **内存倾印**：提供 `hexdump` 以十六进制显示字节流内容。

- **国际化支持**：从本质上支持 `unicode(wchar_t)` 及 `narrow(char)` 字符串输出。

- **线程安全**：通过配置宏参数 `TINYLOG_USE_SINGLE_THREAD` 决定是否启用保护。

## 支持的编译器

直到 2018 年，当前 C++ 已经发展到 C++17，新的版本尚末广泛普及。而 C++11 是发展过程中的一个重大变化的版本，支持了很多新的特性，到目前为止，已被普遍接受。我们目前在以下编译环境进行了验证：

- GCC 5.4.0 / Ubuntu 16.04.3 LTS
- Clang 9.1.0 / MacOS 10.13.4
- Microsoft Visual C++ 2015 / Windows 7

## 整合

只需要让你的工程可以搜索到 [`include`](https://github.com/yanminhui/tinylog/tree/master/include) 目录下的头文件 [`tinylog.hpp`](https://github.com/yanminhui/tinylog/tree/master/include/tinylog.hpp)  即可。这个库没有任何依赖，所有你需要做的就是添加以下代码到你需要输出日志的地方。

~~~cpp
#include "tinylog.hpp"

// 引入命名空间 tinylog
using namespace tinylog;
~~~

另外，别忘记设置启用 C++11 的开关（比如：对于 GCC 设置 `-std=c++11`）。

## 示例

~~~cpp
#include "tinylog.hpp"

#include <map>
#include <string>

int main(int argc, char* argv[])
{
    using namespace tinylog;

    //--------------|
    // 设置          |
    //--------------|
    // 安装输出槽: @see std::make_shared<>
    logger::add_sink<sink::console_sink>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    logger::add_sink<sink::u8_file_sink>("default.log", max_file_size);

    // 过滤日志级别
    logger::set_level(debug);

    // [可选] 输出日志边界
    lout_d << logger::title();

    //--------------|
    // 输出日志       |
    //--------------|
    // 普通文本
    lout(info) << "Weclome to TinyLog !!!" << std::endl;

    // STL容器
    std::map<std::string, size_t> const ages = {{ "tinylog", 1 }, { "json", 5 }};
    lout(warn) << "ages: " << ages << std::endl;

    // 十六进制
    constexpr auto text = "Bravo! The job has been done well.";
    lout(error) << "hexdump: " << text << "\n" << hexdump(text);

    return 0;
}
~~~

> 上述例子存在 [example_narrow](https://github.com/yanminhui/tinylog/tree/master/example/example_narrow.cpp)
> 宽字符版本见 [example_unicode](https://github.com/yanminhui/tinylog/tree/master/example/example_unicode.cpp)

![example_narrow](https://github.com/yanminhui/tinylog/tree/master/example/example_narrow.png)

## 授权

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

## 联系

如果你有关于这个库的问题或建议，请在 [Github 上打开一个 Issue](https://github.com/yanminhui/tinylog/issues/new)，便于大家分享观点，协同解决问题。

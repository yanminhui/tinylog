#include "tinylog.hpp"

#include <locale>
#include <map>
#include <string>

int main(int argc, char* argv[])
{
    using namespace tinylog;

    //--------------|
    // 设置         |
    //--------------|
    std::locale loc("");
    std::locale::global(loc);

    // 安装输出槽
    logger::add_sink<sink::wconsole_sink>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    logger::add_sink<sink::wu8_file_sink>("default.log", max_file_size);

    // 过滤日志级别
    logger::set_level(debug);

    // [可选] 输出日志边界
    wlout_d << logger::wtitle();

    //--------------|
    // 输出日志     |
    //--------------|
    // 普通文本
    wlout(info) << L"Weclome to TinyLog !!!" << std::endl;

    // STL容器
    std::map<std::wstring, size_t> const ages = {{ L"tinylog", 1 }, { L"json", 5 }};
    wlout(warn) << L"ages: " << ages << std::endl;

    // 十六进制
    constexpr auto text = L"Bravo! The job has been done well.";
    wlout(error) << L"hexdump: " << text << L"\n" << whexdump(text);

    return 0;
}

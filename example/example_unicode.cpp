#include <locale>
#include <map>
#include <string>

#include "tinylog.hpp"

int main(int argc, char* argv[])
{
    using namespace tinylog;

    //--------------|
    // 设置         |
    //--------------|
    std::locale loc("");
    std::locale::global(loc);

    // 安装日志记录器
    auto inst = registry::create_logger();

    // 安装输出槽
    inst->create_sink<sink::wconsole_sink>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    inst->create_sink<sink::wu8_file_sink>("default.log", max_file_size);

    // 过滤日志级别
    inst->set_level(debug);

    // [可选] 输出日志边界
    wdlout(inst, debug) << logger::wtitle();

    //--------------|
    // 输出日志     |
    //--------------|
    // 普通文本
    wlout(info) << L"Weclome to TinyLog !!!" << std::endl;

    // STL容器
    std::map<std::wstring, size_t> const ages = {{L"tl", 1}, {L"js", 5}};
    wlout(warn) << L"ages: " << ages << std::endl;

    // 十六进制
    constexpr auto text = L"Bravo! The job has been done well.";
    wlout(error) << L"hexdump: " << text << L"\n" << whexdump(text);

    return 0;
}

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
    
    // 安装输出槽: @see std::make_shared<>
    logger::add_sink<sink::console_sink<default_layout>>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    logger::add_sink<sink::u8_file_sink<>>("default.log", max_file_size);

    // 过滤日志级别
    logger::set_level(debug);

    // [可选] 输出日志边界
    lout_d << logger::title();

    //--------------|
    // 输出日志    |
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

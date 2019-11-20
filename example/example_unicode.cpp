#include <locale>
#include <map>
#include <string>

#include "tinylog/tinylog.hpp"
#include "tinylog/tinylog_extra.hpp"


int main(int argc, char* argv[])
{
    using namespace tinylog;

    //--------------|
    // Setting      |
    //--------------|
    std::locale loc("");
    std::locale::global(loc);

    // Create logger.
    auto inst = registry::create_logger();

    // Set up log sink.
    //
    // @see std::make_shared
    inst->create_sink<sink::wconsole_sink>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    auto sk = inst->create_sink<sink::wu8_file_sink>("default.log", max_file_size);
    sk->enable_verbose(true);

    // Filter log level.
    inst->set_level(debug);

    // Generate log title.
    wdlout(inst, debug) << logger::wtitle();

    //---------------|
    // Print Message |
    //---------------|
    // Normal text.
    wlout_if(info, true) << L"Weclome to TinyLog !!!" << std::endl;

    // Print STL container.
    std::map<std::wstring, size_t> const ages = {{L"tl", 1}, {L"js", 5}};
    wlout(warn) << L"ages: " << ages << std::endl;

    // Print string in hex.
    constexpr auto text = L"Bravo! The job has been done well.";
    wlout(error) << L"hexdump: " << text << L"\n" << whexdump(text);

    return 0;
}

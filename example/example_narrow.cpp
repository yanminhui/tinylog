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
    // @see std::make_shared<>
    inst->create_sink<sink::console_sink>();

    constexpr auto max_file_size = 5 * 1024 * 1024; // 5MB
    auto sk = inst->create_sink<sink::file_sink>("default.log", max_file_size);
    sk->enable_verbose(true);

    // Filter log level.
    inst->set_level(debug);

    // Print title.
    dlout(inst, debug) << logger::title();

    //----------------|
    // Print Message  |
    //----------------|
    // Normal text.
    lout_if(info, true) << "Weclome to TinyLog !!!" << std::endl;

    // STL container.
    std::map<std::string, size_t> const ages = {{"tl", 1}, {"js", 5}};
    lout(warn) << "ages: " << ages << std::endl;

    // Print string in hex.
    constexpr auto text = "Bravo! The job has been done well.";
    lout(error) << "hexdump: " << text << "\n" << hexdump(text);

    return 0;
}

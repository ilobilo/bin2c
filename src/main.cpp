// Copyright (C) 2022  ilobilo

#include <conflict/conflict.hpp>

#include <filesystem>
#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

std::vector<std::string_view> files;
uint64_t flags = 0;

const auto parser = conflict::parser
{
    conflict::option { { 'h', "help", "Show help" }, flags, (1 << 0) },
    conflict::option { { 'v', "version", "Show version" }, flags, (1 << 1) }
};

void usage(bool err)
{
    auto &out = (err ? std::cerr : std::cout);
    out << "Usage:\n";
    out << "    bin2c input.bin output.h\n";
}

bool parse_flags()
{
    if (flags & (1 << 0))
    {
        usage(false);
        std::cout << "Options:\n";
        parser.print_help();
        return true;
    }
    else if (flags & (1 << 1))
    {
        std::cout << "bin2c v0.1\n";
        return true;
    }

    return false;
}

auto main(int argc, char **argv) -> int
{
    parser.apply_defaults();
    conflict::default_report(parser.parse(argc - 1, argv + 1, files));

    if (parse_flags())
        return EXIT_SUCCESS;

    if (files.size() != 2)
    {
        usage(true);
        return EXIT_FAILURE;
    }

    auto input_file = files.front();
    auto output_file = files.back();

    if (fs::exists(input_file) == false)
    {
        std::cerr << "File '" << input_file << "' does not exist!" << std::endl;
        return EXIT_FAILURE;
    }

    if (fs::is_regular_file(input_file) == false)
    {
        std::cerr << "'" << input_file << "' is not a regular file!" << std::endl;
        return EXIT_FAILURE;
    }

    if (fs::exists(output_file) && fs::is_regular_file(output_file) == false)
    {
        std::cerr << "'" << output_file << "' is not a regular file!" << std::endl;
        return EXIT_FAILURE;
    }

    std::string name = fs::path(output_file).stem();
    std::replace_if(name.begin(), name.end(), [](char c) { return !(std::isalnum(c) || c == '_'); }, '_');

    std::string upname(name);
    std::transform(upname.begin(), upname.end(), upname.begin(), ::toupper);

    std::string macro_name(upname);
    macro_name += "_H";

    std::ifstream input(input_file.data(), std::ios::binary);
    std::ofstream output(output_file.data(), std::ios::trunc);

    output << "#ifndef " << macro_name << "\n";
    output << "#define " << macro_name << "\n\n";
    output << "#include <stdint.h>\n\n";
    output << "static const uint8_t " << name << "[] = {";

    size_t counter = 0;
    bool first = true;

    for (auto byte = input.get(); byte != EOF; byte = input.get())
    {
        if (counter++ == 0)
        {
            if (first == true)
            {
                first = false;
                output << "\n    ";
            }
            else output << ",\n    ";
        }
        else output << ", ";

        output << "0x" << std::setfill('0') << std::setw(2) << std::hex << byte;

        if (counter == 16)
            counter = 0;
    }

    output << "\n};\n\n";
    output << "#endif // " << macro_name;

    input.close();
    output.close();

    return EXIT_SUCCESS;
}
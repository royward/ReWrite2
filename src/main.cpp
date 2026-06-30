//#include "lexer.hpp"
//#include "parser.hpp"
#include "program.hpp"
#include <CLI/CLI.hpp>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <format>
#include <print>

//Program do_parse(std::string_view program_string);
//std::vector<DataElement> run(const Program& prog, std::string& fn, std::vector<DataElement>& args);

std::string load_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open file: {}", path.string()));
    }
    const auto size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(size));
    return content;
}

int main(int argc, char** argv) {
    CLI::App app{"ReWrite Stage 1 interpreter"};
    std::string filename;
    std::string callname;
    app.add_option("file", filename, "Source file to interpret")->required();
    app.add_option("call", callname, "Funtion to run")->required();
    CLI11_PARSE(app, argc, argv);
    try {
        std::string s=load_file(filename);
        std::println("{}",s);
        Program prog(s);
        std::vector<DataElement> inputs;
        inputs.push_back(DataElement{DataInt{10}});
        std::vector<DataElement> results=prog.run(callname,inputs);
        std::println("");
    } catch(const std::runtime_error& e) {
        std::println("Error: {}", e.what());
    }
}

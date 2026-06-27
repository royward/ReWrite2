#include "lexer.hpp"
#include "parser.hpp"
#include <CLI/CLI.hpp>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <format>
#include <print>

Program do_parse(std::string_view program_string);

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
    app.add_option("file", filename, "Source file to interpret")->required();
    CLI11_PARSE(app, argc, argv);
    try {
        std::string s=load_file(filename);
        std::println("{}",s);
        std::vector<Token> tokens=lex(s);
        for (const auto& token : tokens) {
            std::println("{}", token);
        }
        Program prog=do_parse(s);
        std::println("");
    } catch(const std::runtime_error& e) {
        std::println("Error: {}", e.what());
    }
}

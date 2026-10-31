#include <iostream>
#include <string>
#include "search_engine.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: router-search <query> [root_dir] [max_results]\n"
                  << "  query       : Search query (supports @file, @content, path substrings)\n"
                  << "  root_dir    : Root directory to search (default: \".\")\n"
                  << "  max_results : Maximum matches to return (default: 50)\n";
        return 1;
    }

    std::string query = argv[1];
    std::string root_dir = (argc >= 3) ? argv[2] : ".";
    int max_results = (argc >= 4) ? std::stoi(argv[3]) : 50;

    const char* results_json = synvim_search_repo(root_dir.c_str(), query.c_str(), nullptr, max_results);
    if (!results_json) {
        std::cerr << "Error: Search failed or returned null\n";
        return 1;
    }

    std::cout << results_json << "\n";
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <unistd.h>
#include "search_engine.h"

// ANSI colors for terminal output
static const char* COLOR_RESET   = "\033[0m";
static const char* COLOR_PATH    = "\033[36m";      // cyan
static const char* COLOR_LINE    = "\033[33m";      // yellow
static const char* COLOR_TAG     = "\033[35m";      // magenta
static const char* COLOR_DESC    = "\033[1m";       // bold

struct Match {
    std::string path;
    int line = 1;
    int col = 1;
    double score = 0.0;
    std::string match_type;
    std::string description;
};

// Lightweight JSON unescaper and field extractor
static std::string unescape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[++i];
            if (next == 'n') out += '\n';
            else if (next == 't') out += '\t';
            else if (next == 'r') out += '\r';
            else if (next == '"') out += '"';
            else if (next == '\\') out += '\\';
            else out += next;
        } else {
            out += s[i];
        }
    }
    return out;
}

static std::vector<Match> parse_results_json(const std::string& json_str) {
    std::vector<Match> matches;
    size_t pos = 0;

    while ((pos = json_str.find('{', pos)) != std::string::npos) {
        size_t end = json_str.find('}', pos);
        if (end == std::string::npos) break;

        std::string obj = json_str.substr(pos + 1, end - pos - 1);
        Match m;

        // Extract "path":"..."
        size_t k = obj.find("\"path\":\"");
        if (k != std::string::npos) {
            k += 8;
            size_t e = obj.find('"', k);
            if (e != std::string::npos) m.path = unescape_json(obj.substr(k, e - k));
        }

        // Extract "line":...
        k = obj.find("\"line\":");
        if (k != std::string::npos) {
            k += 7;
            size_t e = obj.find_first_of(",}", k);
            if (e != std::string::npos) m.line = std::stoi(obj.substr(k, e - k));
        }

        // Extract "col":...
        k = obj.find("\"col\":");
        if (k != std::string::npos) {
            k += 6;
            size_t e = obj.find_first_of(",}", k);
            if (e != std::string::npos) m.col = std::stoi(obj.substr(k, e - k));
        }

        // Extract "match_type":"..."
        k = obj.find("\"match_type\":\"");
        if (k != std::string::npos) {
            k += 14;
            size_t e = obj.find('"', k);
            if (e != std::string::npos) m.match_type = unescape_json(obj.substr(k, e - k));
        }

        // Extract "description":"..."
        k = obj.find("\"description\":\"");
        if (k != std::string::npos) {
            k += 15;
            size_t e = obj.find('"', k);
            if (e != std::string::npos) m.description = unescape_json(obj.substr(k, e - k));
        }

        if (!m.path.empty()) {
            matches.push_back(m);
        }

        pos = end + 1;
    }

    return matches;
}

static void print_usage() {
    std::cerr << "router-search — Fast hybrid code & symbol search\n\n"
              << "Usage:\n"
              << "  router-search <query> [dir] [max_results]\n"
              << "  router-search [options] <query>\n\n"
              << "Options:\n"
              << "  -d, --dir <path>       Directory to search (default: \".\")\n"
              << "  -n, --max <count>      Maximum results to return (default: 30)\n"
              << "  -l, --files-only       Print only matching file paths\n"
              << "      --json             Output raw JSON results\n"
              << "  -h, --help             Show this help message\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string query;
    std::string root_dir = ".";
    int max_results = 30;
    bool json_output = false;
    bool files_only = false;

    // Argument parsing
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "--json") {
            json_output = true;
        } else if (arg == "-l" || arg == "--files-only" || arg == "--files") {
            files_only = true;
        } else if ((arg == "-d" || arg == "--dir") && i + 1 < argc) {
            root_dir = argv[++i];
        } else if ((arg == "-n" || arg == "--max") && i + 1 < argc) {
            max_results = std::stoi(argv[++i]);
        } else if (!arg.empty() && arg[0] != '-') {
            positional.push_back(arg);
        }
    }

    if (positional.empty()) {
        std::cerr << "Error: No search query specified.\n";
        print_usage();
        return 1;
    }

    query = positional[0];
    if (positional.size() >= 2 && root_dir == ".") {
        root_dir = positional[1];
    }
    if (positional.size() >= 3) {
        max_results = std::stoi(positional[2]);
    }

    const char* results_json = synvim_search_repo(root_dir.c_str(), query.c_str(), nullptr, max_results);
    if (!results_json) {
        std::cerr << "Error: Search failed or returned null\n";
        return 1;
    }

    if (json_output) {
        std::cout << results_json << "\n";
        return 0;
    }

    auto matches = parse_results_json(results_json);

    if (files_only) {
        std::set<std::string> unique_files;
        for (const auto& m : matches) {
            if (unique_files.insert(m.path).second) {
                std::cout << m.path << "\n";
            }
        }
        return 0;
    }

    bool is_tty = isatty(STDOUT_FILENO);
    const char* c_path = is_tty ? COLOR_PATH : "";
    const char* c_line = is_tty ? COLOR_LINE : "";
    const char* c_tag  = is_tty ? COLOR_TAG : "";
    const char* c_desc = is_tty ? COLOR_DESC : "";
    const char* c_rst  = is_tty ? COLOR_RESET : "";

    for (const auto& m : matches) {
        std::cout << c_path << m.path << c_rst << ":"
                  << c_line << m.line << c_rst << ":"
                  << c_line << m.col << c_rst << " ";
        if (!m.match_type.empty() && m.match_type != "content") {
            std::cout << c_tag << "[" << m.match_type << "]" << c_rst << " ";
        }
        std::cout << c_desc << m.description << c_rst << "\n";
    }

    return 0;
}

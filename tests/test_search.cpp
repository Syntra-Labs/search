#include <cassert>
#include <cstring>
#include <iostream>
#include "search_engine.h"

int main() {
    std::cout << "Testing router hybrid search engine...\n";

    // 1. Version test
    const char* version = synvim_search_version();
    assert(version != nullptr);
    assert(std::strlen(version) > 0);
    std::cout << "  • Engine version: " << version << " [PASS]\n";

    // 2. Query test in current directory
    const char* results = synvim_search_repo(".", "manifold", nullptr, 10);
    assert(results != nullptr);
    (void)results;
    std::cout << "  • Repo query returned non-null JSON [PASS]\n";

    std::cout << "All search engine unit tests passed successfully!\n";
    return 0;
}

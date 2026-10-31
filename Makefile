CXX ?= g++
CXXFLAGS ?= -O3 -march=native -std=c++20 -Wall -Wextra -fPIC -Iinclude -Isrc

BUILD_DIR ?= build
BIN_DIR ?= bin
LIB_DIR ?= lib

TARGET_MANIFOLD = $(BIN_DIR)/manifold
TARGET_SEARCH_LIB = $(LIB_DIR)/librouter_search.so
TARGET_SEARCH_CLI = $(BIN_DIR)/router-search
TARGET_SEARCH_TEST = $(BIN_DIR)/test_search

.PHONY: all clean test install

all: $(TARGET_MANIFOLD) $(TARGET_SEARCH_LIB) $(TARGET_SEARCH_CLI) $(TARGET_SEARCH_TEST)

$(BIN_DIR) $(LIB_DIR):
	mkdir -p $@

$(TARGET_MANIFOLD): src/manifold.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@
	@echo "Built $@"

$(TARGET_SEARCH_LIB): src/search_engine.cpp src/search_engine.h | $(LIB_DIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ src/search_engine.cpp
	ln -sf librouter_search.so $(LIB_DIR)/libsynvim_search.so
	@echo "Built $@"

$(TARGET_SEARCH_CLI): src/search_cli.cpp $(TARGET_SEARCH_LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lrouter_search -Wl,-rpath,'$$ORIGIN/../lib' -o $@
	@echo "Built $@"

$(TARGET_SEARCH_TEST): tests/test_search.cpp $(TARGET_SEARCH_LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lrouter_search -Wl,-rpath,'$$ORIGIN/../lib' -o $@
	@echo "Built $@"

test: all
	@echo "=== Running Manifold Selftest ==="
	$(TARGET_MANIFOLD) selftest
	@echo "=== Running Search Engine Unit Tests ==="
	$(TARGET_SEARCH_TEST)
	@echo "All tests passed!"

install: all
	@mkdir -p $(HOME)/.local/bin $(HOME)/.local/lib $(HOME)/.local/include/router
	cp $(TARGET_MANIFOLD) $(HOME)/.local/bin/
	cp $(TARGET_SEARCH_CLI) $(HOME)/.local/bin/
	cp $(TARGET_SEARCH_LIB) $(HOME)/.local/lib/
	cp $(LIB_DIR)/libsynvim_search.so $(HOME)/.local/lib/
	cp include/router/*.h $(HOME)/.local/include/router/
	chmod +x $(HOME)/.local/bin/manifold $(HOME)/.local/bin/router-search
	@echo "Installed binaries to $(HOME)/.local/bin/ and libraries to $(HOME)/.local/lib/"

clean:
	rm -rf $(BIN_DIR) $(LIB_DIR) $(BUILD_DIR)

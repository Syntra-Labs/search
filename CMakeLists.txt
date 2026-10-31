cmake_minimum_required(VERSION 3.20)
project(router VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and Warning Flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native)
endif()

include_directories(include src)

# 1. Search Engine Library (Shared & Static)
add_library(router_search SHARED src/search_engine.cpp)
target_include_directories(router_search PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(router_search PROPERTIES OUTPUT_NAME "router_search")

add_library(router_search_static STATIC src/search_engine.cpp)
target_include_directories(router_search_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Compatibility alias for synvim
add_custom_target(synvim_search_symlink ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:router_search>
        ${CMAKE_BINARY_DIR}/libsynvim_search.so
    DEPENDS router_search
)

# 2. Standalone Manifold CLI / Substrate
add_executable(manifold src/manifold.cpp)

# 3. Router Search CLI
add_executable(router-search src/search_cli.cpp)
target_link_libraries(router-search PRIVATE router_search_static)

# 4. Testing
enable_testing()

add_executable(test_search tests/test_search.cpp)
target_link_libraries(test_search PRIVATE router_search_static)

add_test(NAME ManifoldSelftest COMMAND manifold selftest)
add_test(NAME SearchEngineTest COMMAND test_search)

# Installation Rules
include(GNUInstallDirs)
install(TARGETS router_search router_search_static manifold router-search
    EXPORT routerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

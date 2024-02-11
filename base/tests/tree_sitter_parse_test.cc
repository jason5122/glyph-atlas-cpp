#include "util/file_util.h"
#include "gtest/gtest.h"
#include <iostream>
#include <tree_sitter/api.h>

extern "C" TSLanguage* tree_sitter_json();

struct buffer {
    const char* buf;
    size_t len;
};

const char* read(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read) {
    if (byte_index >= ((buffer*)payload)->len) {
        *bytes_read = 0;
        return (char*)"";
    } else {
        *bytes_read = 1;
        return (char*)(((buffer*)payload)->buf) + byte_index;
    }
}

// TODO: Add actual tests to this test case.
TEST(TreeSitterParserTest, Json) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    std::string source_code = ReadFile("example.json");
    buffer buf = {&source_code[0], source_code.size()};
    TSInput input = {&buf, read, TSInputEncodingUTF8};

    TSTree* tree = ts_parser_parse(parser, NULL, input);
    TSNode root_node = ts_tree_root_node(tree);
    std::cerr << ts_node_string(root_node) << '\n';
}

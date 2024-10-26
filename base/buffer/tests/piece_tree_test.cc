#include "base/buffer/fredbuf.h"
#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include <gtest/gtest.h>
#include <random>

#include "util/std_print.h"

namespace {

std::mt19937 rng{std::random_device{}()};

// Returns a random number in the range [low, high].
inline int RandomNumber(int low, int high) {
    std::uniform_int_distribution<> distr{low, high};
    return distr(rng);
}

// Returns a random alphanumeric string of a specified length.
inline std::string RandomString(size_t length) {
    static constexpr std::string_view charset = "0123456789"
                                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz";
    auto random_char = []() -> char { return charset.at(RandomNumber(0, charset.length() - 1)); };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, random_char);
    return str;
}

// Like `RandomString()`, but the string is guaranteed to contain a specified number of newlines.
inline std::string RandomNewlineString(size_t length, size_t newlines) {
    std::string str = RandomString(base::sub_sat(length, newlines));
    for (size_t k = 0; k < newlines; ++k) {
        size_t i = RandomNumber(0, str.length());
        str.insert(i, "\n");
    }
    return str;
}

}

namespace PieceTree {

TEST(PieceTreeTest, CustomTest1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, CustomTest2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(9, " and nimble");
    tree.insert(9, " and nimble");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(str.length(), ".");
    tree.insert(tree.length(), ".");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, CustomTest3) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(9, " and nimble");
    tree.insert(9, " and nimble");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(9, 13);
    tree.erase(9, 13);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, CustomTest4) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(9, " and nimble");
    tree.insert(9, " and nimble");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(9, 4);
    tree.erase(9, 4);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, Init) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtBeginningOfString1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    tree.insert(0, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(0, s1);
    tree.insert(0, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtBeginningOfString2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "String1 ";
    for (size_t n = 0; n < 5; ++n) {
        str.insert(0, s1);
        tree.insert(0, s1);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());
    }
}

TEST(PieceTreeTest, InsertAtBeginningOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    tree.insert(0, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s2 = "String2 ";
    str.insert(s1.length(), s2);
    tree.insert(s1.length(), s2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtBeginningOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    tree.insert(0, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s2 = "String2 ";
    str.insert(s1.length(), s2);
    tree.insert(s1.length(), s2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s3 = "String3 ";
    str.insert(s1.length(), s3);
    tree.insert(s1.length(), s3);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s4 = "String4 ";
    str.insert(s1.length() + s2.length(), s4);
    tree.insert(s1.length() + s2.length(), s4);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtMiddleOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    tree.insert(20, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    tree.insert(9, s2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtMiddleOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    tree.insert(20, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    tree.insert(9, s2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s3 = " sneaky,";
    str.insert(3, s3);
    tree.insert(3, s3);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s4 = "String4";
    str.insert(30, s4);
    tree.insert(30, s4);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s5 = "String5";
    str.insert(15, s5);
    tree.insert(15, s5);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtEnd1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = " and walked away\n";
    size_t len = str.length();
    str.insert(len, s1);
    tree.insert(len, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertAtEnd2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = " and walked away\n";
    size_t len1 = str.length();
    str.insert(len1, s1);
    tree.insert(len1, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s2 = "went to the park and\n";
    str.insert(20, s2);
    tree.insert(20, s2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s3 = " and nimble";
    str.insert(9, s3);
    tree.insert(9, s3);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s4 = " from the dog\n";
    size_t len2 = str.length();
    str.insert(len2, s4);
    tree.insert(len2, s4);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertCustomTest1) {
    std::string str = "";
    Tree tree{str};

    const std::string s1 = "R7";
    str.insert(0, s1);
    tree.insert(0, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s2 = "nB";
    str.insert(1, s2);
    tree.insert(1, s2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    const std::string s3 = "6D";
    str.insert(2, s3);
    tree.insert(2, s3);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, InsertEmpty) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.insert(0, "");
    tree.insert(0, "");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(10, "");
    tree.insert(10, "");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(18, "");
    tree.insert(18, "");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.insert(str.length(), "");
    tree.insert(str.length(), "");
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

// Randomly inserts 100 alphanumeric strings of length [0, 10] at index [0, length).
TEST(PieceTreeTest, InsertAtRandom) {
    std::string str = "";
    Tree tree{str};

    for (size_t n = 0; n < 100; ++n) {
        size_t index = RandomNumber(0, str.length());
        const std::string random_str = RandomString(RandomNumber(0, 10));

        str.insert(index, random_str);
        tree.insert(index, random_str);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());

        // Insert at beginning of piece.
        str.insert(0, random_str);
        tree.insert(0, random_str);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());

        // Insert at end of piece.
        str.insert(str.length(), random_str);
        tree.insert(tree.length(), random_str);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());
    }
}

TEST(PieceTreeTest, EraseAtBeginning1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    while (str.length() > 0) {
        str.erase(0, 1);
        tree.erase(0, 1);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());
    }
}

TEST(PieceTreeTest, EraseAtMiddleOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.erase(3, 6);
    tree.erase(3, 6);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(4, 10);
    tree.erase(4, 10);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseAtMiddleOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.erase(3, 6);
    tree.erase(3, 6);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(4, 10);
    tree.erase(4, 10);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(4, 21);
    tree.erase(4, 21);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseBeyondOnePiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = " and nimble";
    str.insert(9, s1);
    tree.insert(9, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(4, 17);
    tree.erase(4, 17);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseBeyondOnePiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "String1";
    str.insert(0, s1);
    tree.insert(0, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(1, str.length());
    tree.erase(1, tree.length());
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseBeyondOnePiece3) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    const std::string s1 = "String1";
    str.insert(10, s1);
    tree.insert(10, s1);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(0, 9999);
    tree.erase(0, 9999);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseCustomTest1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.erase(25, 44);
    tree.erase(25, 44);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(9, 14);
    tree.erase(9, 14);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseCustomTest2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.erase(3, 2);
    tree.erase(3, 2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(7, 15);
    tree.erase(7, 15);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(4, 4);
    tree.erase(4, 4);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(22, 7);
    tree.erase(22, 7);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseCustomTest3) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.erase(1, 40);
    tree.erase(1, 40);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(1, 0);
    tree.erase(1, 0);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(1, 2);
    tree.erase(1, 2);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(2, 0);
    tree.erase(2, 0);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, EraseEmpty) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    Tree tree{str};

    str.erase(1, 0);
    tree.erase(1, 0);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(8, 0);
    tree.erase(8, 0);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(0, 0);
    tree.erase(0, 0);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());

    str.erase(str.length(), 0);
    tree.erase(str.length(), 0);
    EXPECT_EQ(str, tree.str());
    EXPECT_EQ(str.length(), tree.length());
}

TEST(PieceTreeTest, RandomTestLite) {
    std::string str = "";
    Tree tree{str};

    for (size_t n = 0; n < 5; ++n) {
        // Randomly insert.
        size_t insert_index = RandomNumber(0, str.length());
        const std::string random_str = RandomString(RandomNumber(0, 10));
        str.insert(insert_index, random_str);
        tree.insert(insert_index, random_str);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());

        // Randomly erase.
        size_t erase_index = RandomNumber(0, str.length());
        size_t count = RandomNumber(0, 4);
        str.erase(erase_index, count);
        tree.erase(erase_index, count);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());
    }
}

// Randomly erases 10 times from the string at index [0, length] and count [0, length).
// We repeat this for 100 iterations.
TEST(PieceTreeTest, EraseAtRandom) {
    constexpr std::string_view original_str = "The quick brown fox\njumped over the lazy dog";

    for (size_t n = 0; n < 100; ++n) {
        std::string str{original_str};
        Tree tree{original_str};

        for (size_t i = 0; i < 10; ++i) {
            size_t index = RandomNumber(0, str.length());
            size_t count = RandomNumber(0, str.length());

            str.erase(index, count);
            tree.erase(index, count);
            EXPECT_EQ(str, tree.str());
            EXPECT_EQ(str.length(), tree.length());
        }
    }
}

TEST(PieceTreeTest, CombinedRandomTest1) {
    std::string str = "";
    Tree tree{str};

    for (size_t n = 0; n < 100; ++n) {
        // Randomly insert.
        size_t insert_index = RandomNumber(0, str.length());
        const std::string random_str = RandomString(RandomNumber(0, 10));
        str.insert(insert_index, random_str);
        tree.insert(insert_index, random_str);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());

        // Randomly erase.
        size_t erase_index = RandomNumber(0, str.length());
        size_t count = RandomNumber(0, 4);
        str.erase(erase_index, count);
        tree.erase(erase_index, count);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());
    }
}

TEST(PieceTreeTest, LineColumnAt1) {
    auto check = [](std::string_view str) {
        Tree tree{str};

        std::unordered_map<size_t, std::pair<size_t, size_t>> index_to_line_col_map;
        size_t line = 0;
        size_t col = 0;
        for (size_t i = 0; i <= str.length(); i++) {
            index_to_line_col_map[i] = {line, col};

            if (str[i] == '\n') {
                ++line;
                col = 0;
            } else {
                ++col;
            }
        }

        for (size_t i = 0; i <= tree.length(); i++) {
            EXPECT_EQ(tree.line_column_at(i), index_to_line_col_map[i]);
        }
    };

    check("Line 1\nLine 2");
    check("Line 1\nLine 2\nLine 3\nLine 4");
    check("\n\n\n");
}

TEST(PieceTreeTest, LineColumnAt2) {
    std::string str1 = "Hello world!";
    Tree tree1{str1};
    auto [line1, col1] = tree1.line_column_at(99999);
    EXPECT_EQ(line1, 0_Z);
    EXPECT_EQ(col1, tree1.length());

    std::string str2 = "Hello\nworld!";
    Tree tree2{str2};
    auto [line2, col2] = tree2.line_column_at(99999);
    EXPECT_EQ(line2, 1_Z);
    EXPECT_EQ(col2, tree2.length() - 6);
}

TEST(PieceTreeTest, LineColumnAt3) {
    std::string str = "Hello\nworld!\nthis is a newline";
    Tree tree{str};

    str.insert(23, "\n");
    tree.insert(23, "\n");
    EXPECT_EQ(str, tree.str());

    size_t line;
    size_t col;
    std::tie(line, col) = tree.line_column_at(2);
    EXPECT_EQ(line, 0_Z);
    EXPECT_EQ(col, 2_Z);

    std::tie(line, col) = tree.line_column_at(7);
    EXPECT_EQ(line, 1_Z);
    EXPECT_EQ(col, 1_Z);
}

TEST(PieceTreeTest, LineColumnAtRandomTest) {
    std::string str = "";
    Tree tree{str};

    auto check = [](std::string_view str) {
        Tree tree{str};

        std::unordered_map<size_t, std::pair<size_t, size_t>> index_to_line_col_map;
        size_t line = 0;
        size_t col = 0;
        for (size_t i = 0; i <= str.length(); i++) {
            index_to_line_col_map[i] = {line, col};

            if (str[i] == '\n') {
                ++line;
                col = 0;
            } else {
                ++col;
            }
        }

        for (size_t i = 0; i <= tree.length(); i++) {
            EXPECT_EQ(tree.line_column_at(i), index_to_line_col_map[i]);
        }
    };

    for (size_t n = 0; n < 50; ++n) {
        // Randomly insert.
        size_t insert_index = RandomNumber(0, str.length());
        const std::string random_str = RandomNewlineString(RandomNumber(0, 10), 5);
        str.insert(insert_index, random_str);
        tree.insert(insert_index, random_str);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());

        // Randomly erase.
        size_t erase_index = RandomNumber(0, str.length());
        size_t erase_count = RandomNumber(0, 4);
        str.erase(erase_index, erase_count);
        tree.erase(erase_index, erase_count);
        EXPECT_EQ(str, tree.str());
        EXPECT_EQ(str.length(), tree.length());

        check(str);
    }
}

}

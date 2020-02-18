#include "./cmdline/cmdline.hpp"
#include <gtest/gtest.h>

#include <vector>

namespace po=program_options;

struct program_options_Test: public ::testing::Test {
    using string=std::string;
    po::params_map params;
    
    program_options_Test() {
        static const char* argv[] = {"program.x", "a_number=1111", "an_option=false", "a_string=hello"};
        const int argc = sizeof(argv)/sizeof(*argv);
        auto maybe_params = po::parse(argc, argv);
        params = *maybe_params;
    }
    
    
};


TEST_F(program_options_Test, wrongParams) {
    static const char* argv[] = {"program.x", "something", "a_number=1111",
                                 "an_option=false", "a_string=hello"};
    const int argc = sizeof(argv)/sizeof(*argv);
    auto maybe_params = po::parse(argc, argv);
    EXPECT_FALSE(bool(maybe_params));
}

TEST_F(program_options_Test, derefParams) {
    static const char* argv[] = {"program.x", "a_number=1111"};
    const int argc = sizeof(argv)/sizeof(*argv);
    auto maybe_params = po::parse(argc, argv);
    ASSERT_TRUE(bool(maybe_params));
    auto maybe_number = maybe_params->get<int>("a_number");
    EXPECT_EQ(1111,*maybe_number);
}


TEST_F(program_options_Test, intNumber) {
    auto maybe_number=params.get<int>("a_number");
    ASSERT_TRUE(bool(maybe_number));
    EXPECT_EQ(1111, *maybe_number);
}


TEST_F(program_options_Test, intNumberDefault) {
    auto maybe_number=params.get_or("a_number",7777);
    ASSERT_TRUE(bool(maybe_number));
    EXPECT_EQ(1111, *maybe_number);
}


TEST_F(program_options_Test, intNumberAsString) {
    auto maybe_string=params.get<string>("a_number");
    ASSERT_TRUE(bool(maybe_string));
    EXPECT_EQ("1111", *maybe_string);
}


TEST_F(program_options_Test, intNumberAsStringDefault) {
    auto maybe_string=params.get_or("a_number", "7777");
    ASSERT_TRUE(bool(maybe_string));
    EXPECT_EQ("1111", *maybe_string);
}


TEST_F(program_options_Test, stringParam) {
    auto maybe_string=params.get<string>("a_string");
    ASSERT_TRUE(bool(maybe_string));
    EXPECT_EQ("hello", *maybe_string);
}


TEST_F(program_options_Test, stringParamDefault) {
    auto maybe_string=params.get_or("a_string", "bye");
    ASSERT_TRUE(bool(maybe_string));
    EXPECT_EQ("hello", *maybe_string);
}


TEST_F(program_options_Test, stringAsNumber) {
    auto maybe_number=params.get<int>("a_string");
    EXPECT_FALSE(bool(maybe_number));
    EXPECT_ANY_THROW(*maybe_number);
}


TEST_F(program_options_Test, stringAsNumberDefault) {
    auto maybe_number=params.get_or("a_string", 7777);
    EXPECT_FALSE(bool(maybe_number));
    EXPECT_ANY_THROW(*maybe_number);
}


TEST_F(program_options_Test, missingIntNumber) {
    auto maybe_number=params.get<int>("missing");
    EXPECT_FALSE(bool(maybe_number));
    EXPECT_ANY_THROW(*maybe_number);
}


TEST_F(program_options_Test, missingIntNumberDefault) {
    auto maybe_number=params.get_or("missing",7777);
    ASSERT_TRUE(bool(maybe_number));
    EXPECT_EQ(7777, *maybe_number);
}


TEST_F(program_options_Test, missingStringDefault) {
    auto maybe_string=params.get_or("missing","bye");
    ASSERT_TRUE(bool(maybe_string));
    EXPECT_EQ("bye", *maybe_string);
}


TEST_F(program_options_Test, boolParam) {
    auto maybe_option=params.get<bool>("an_option");
    ASSERT_TRUE(bool(maybe_option));
    EXPECT_FALSE(*maybe_option);
}


TEST_F(program_options_Test, boolParamDefault) {
    auto maybe_option=params.get_or("an_option", true);
    ASSERT_TRUE(bool(maybe_option));
    EXPECT_FALSE(*maybe_option);
}


TEST_F(program_options_Test, missingBoolDefault) {
    auto maybe_option=params.get_or("missing", false);
    ASSERT_TRUE(bool(maybe_option));
    EXPECT_FALSE(*maybe_option);
}


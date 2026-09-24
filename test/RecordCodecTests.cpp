#include <gtest/gtest.h>

#include "storage/RecordCodec.hpp"

#include <fstream>
#include <filesystem>

TEST(RecordCodecTest, PutRecordRoundTrip)
{
    const std::filesystem::path path =
        "codec_test.db";

    RecordCodec codec;

    Record original{
        RecordType::put,
        "fruit",
        "apple"
    };

    {
        std::ofstream file(
            path,
            std::ios::binary | std::ios::trunc
        );

        ASSERT_TRUE(file.is_open());

        codec.write_one_record(file, original);
    }

    {
        std::ifstream file(
            path,
            std::ios::binary
        );

        ASSERT_TRUE(file.is_open());

        auto result =
            codec.read_one_record(file);

        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(result->type, RecordType::put);
        EXPECT_EQ(result->key, "fruit");
        EXPECT_EQ(result->value, "apple");
    }

    std::filesystem::remove(path);
}

TEST(RecordCodecTest, RemoveRecordRoundTrip)
{
    const std::filesystem::path path =
        "codec_test.db";

    RecordCodec codec;

    Record original{
        RecordType::remove,
        "fruit",
        ""
    };

    {
        std::ofstream file(
            path,
            std::ios::binary | std::ios::trunc
        );

        ASSERT_TRUE(file.is_open());

        codec.write_one_record(file, original);
    }

    {
        std::ifstream file(
            path,
            std::ios::binary
        );

        ASSERT_TRUE(file.is_open());

        auto result =
            codec.read_one_record(file);

        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(result->type, RecordType::remove);
        EXPECT_EQ(result->key, "fruit");
        EXPECT_EQ(result->value, "");
    }

    std::filesystem::remove(path);
}

TEST(RecordCodecTest, PutSeveralRecordRoundTrip)
{
    const std::filesystem::path path =
        "codec_test.db";

    RecordCodec codec;

    Record A{
        RecordType::put,
        "A",
        "10"
    };
    Record B{
        RecordType::put,
        "B",
        "20"
    };
    Record removeA{
        RecordType::remove,
        "A",
        ""
    };

    {
        std::ofstream file(
            path,
            std::ios::binary | std::ios::trunc
        );

        ASSERT_TRUE(file.is_open());

        bool error_flag = false;
        try {
            codec.write_one_record(file, A);
            codec.write_one_record(file, B);
            codec.write_one_record(file, removeA);
        }
        catch (const RecordCodecException& error) {
            error_flag = true;
        }
        ASSERT_FALSE(error_flag);
    }

    {
        std::ifstream file(
            path,
            std::ios::binary
        );

        ASSERT_TRUE(file.is_open());
        auto result =
            codec.read_one_record(file);

        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(result->type, RecordType::put);
        EXPECT_EQ(result->key, "A");
        EXPECT_EQ(result->value, "10");

        result =
            codec.read_one_record(file);

        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(result->type, RecordType::put);
        EXPECT_EQ(result->key, "B");
        EXPECT_EQ(result->value, "20");

        result =
            codec.read_one_record(file);

        ASSERT_TRUE(result.has_value());

        EXPECT_EQ(result->type, RecordType::remove);
        EXPECT_EQ(result->key, "A");
        EXPECT_EQ(result->value, "");

        result =
            codec.read_one_record(file);

        EXPECT_FALSE(result.has_value());
    }

    std::filesystem::remove(path);
}
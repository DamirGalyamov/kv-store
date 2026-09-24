#include <gtest/gtest.h>

#include "storage/KVStore.hpp"

#include <fstream>
#include <filesystem>

class KVStoreTest : public ::testing::Test
{
protected:
    std::filesystem::path path =
        "kv_store_test.db";

    void SetUp() override
    {
        std::filesystem::remove(path);
    }

    void TearDown() override
    {
        std::filesystem::remove(path);
        std::filesystem::remove(
            path.string() + ".tmp"
        );
    }
};

TEST_F(KVStoreTest, PutThenGet) {
    KVStore store(path.string());

    EXPECT_EQ(
        store.put("A", "10"),
        StoreStatus::ok
    );

    GetResult result = store.get("A");
    ASSERT_EQ(
        result.status,
        StoreStatus::ok
    );

    ASSERT_TRUE(result.value.has_value());

    EXPECT_EQ(
        result.value.value(),
        "10"
    );
}

TEST_F(KVStoreTest, GetMissingKey)
{
    KVStore store(path.string());

    GetResult result = store.get("A");

    ASSERT_EQ(
        result.status,
        StoreStatus::not_found
    );
}

TEST_F(KVStoreTest, OverwriteExistingKey)
{
    KVStore store(path.string());

    EXPECT_EQ(
        store.put("A", "10"),
        StoreStatus::ok
    );

    EXPECT_EQ(
        store.put("A", "20"),
        StoreStatus::ok
    );

    GetResult result = store.get("A");

    ASSERT_EQ(
        result.status,
        StoreStatus::ok
    );

    ASSERT_TRUE(result.value.has_value());

    EXPECT_EQ(
        result.value.value(),
        "20"
    );
}

TEST_F(KVStoreTest, DeleteExistingKey)
{
    KVStore store(path.string());

    EXPECT_EQ(
        store.put("A", "10"),
        StoreStatus::ok
    );

    ASSERT_EQ(
        store.remove("A"),
        StoreStatus::ok
    );

    GetResult result = store.get("A");

    ASSERT_EQ(
        result.status,
        StoreStatus::not_found
    );
}

TEST_F(KVStoreTest, DeleteMissingKey)
{
    KVStore store(path.string());

    EXPECT_EQ(
        store.put("A", "10"),
        StoreStatus::ok
    );

    EXPECT_EQ(
        store.remove("A"),
        StoreStatus::ok
    );

    EXPECT_EQ(
        store.remove("A"),
        StoreStatus::not_found
    );

    GetResult result = store.get("A");

    ASSERT_EQ(
        result.status,
        StoreStatus::not_found
    );
}

TEST_F(KVStoreTest, RejectEmptyKey)
{
    KVStore store(path.string());

    ASSERT_EQ(
        store.put("", "10"),
        StoreStatus::invalid_argument
    );

    GetResult result = store.get("");

    ASSERT_EQ(
        result.status,
        StoreStatus::invalid_argument
    );
}

TEST_F(KVStoreTest, RejectEmptyValue)
{
    KVStore store(path.string());

    ASSERT_EQ(
        store.put("A", ""),
        StoreStatus::invalid_argument
    );

    GetResult result = store.get("A");

    ASSERT_EQ(
        result.status,
        StoreStatus::not_found
    );
}

TEST_F(KVStoreTest, DataSurvivesRestart)
{
    {
        KVStore store(path.string());

        ASSERT_EQ(
            store.put("A", "10"),
            StoreStatus::ok
        );
        ASSERT_EQ(
            store.put("B", "20"),
            StoreStatus::ok
        );
        ASSERT_EQ(
            store.put("C", "30"),
            StoreStatus::ok
        );
    }
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    ASSERT_FALSE(ec);

    std::filesystem::resize_file(path, size - 5, ec);

    ASSERT_FALSE(ec);

    KVStore store(path.string());

    GetResult result = store.get("A");
    ASSERT_EQ(
        result.status,
        StoreStatus::ok
    );
    result = store.get("B");
    ASSERT_EQ(
        result.status,
        StoreStatus::ok
    );
    result = store.get("C");
    ASSERT_EQ(
        result.status,
        StoreStatus::not_found
    );
}
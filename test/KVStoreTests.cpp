#include <gtest/gtest.h>

#include "storage/KVStore.hpp"

#include <fstream>
#include <filesystem>
#include <functional>
#include <thread>
#include <vector>

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

TEST_F(KVStoreTest, DataSurvivesAfterCorruptedData)
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

TEST_F(KVStoreTest, ConcurrentWriteWithDifferentKeys) {
    KVStore store(path.string());

    auto write_work = [](KVStore& store, int thread_num, char& all_ok) {
        StoreStatus status;
        std::string sthread_num = std::to_string(thread_num);
        for (int i = 0; i < 1000; i++) {
            std::string key = "thread_" + sthread_num + "key" + std::to_string(i);
            status = store.put(key, std::to_string(thread_num));
            if (status != StoreStatus::ok) {
                return;
            }
            }
        all_ok = 1;
        };
    int count = 8;
    std::vector<std::thread> threads;
    std::vector<char> all_ok(count, 0);
    for (int i = 0; i < count; i++) {
        threads.emplace_back(write_work, std::ref(store), i, std::ref(all_ok[i]));
    }
    
    for (int i = 0; i < count; i++) {
        threads[i].join();
    }
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(all_ok[i]);
    }

    GetResult result;
    for (int i = 0; i < count; i++) {
        std::string sthread_num = std::to_string(i);
        for (int j = 0; j < 1000; j++) {
            std::string key = "thread_" + sthread_num + "key" + std::to_string(j);
            result = store.get(key);
            if (result.status != StoreStatus::ok || !result.value.has_value() || result.value.value() != sthread_num) {
                all_ok[i] = 0;
                break;
            }
        }
        if (!all_ok[i]) {
            break;
        }
    }

    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(all_ok[i]);
    }
}

TEST_F(KVStoreTest, ConcurrentRead) {
    KVStore store(path.string());
    int count = 8;
    std::vector<std::thread> threads;
    std::vector<char> all_ok(count, 0);
    StoreStatus status;
    
    for (int i = 0; i < 100; ++i) {
        status = store.put("key_" + std::to_string(i), "value_" + std::to_string(i));
        if (status != StoreStatus::ok) {
            all_ok[0] = 1;
        }
    }
    ASSERT_FALSE(all_ok[0]);
    all_ok[0] = 0;

    auto read_work = [](KVStore& store, int thread_num, char& all_ok) {
        GetResult result;
        for (int i = 0; i < 100; i++) {
            std::string key = "key_" + std::to_string(i);
            result = store.get(key);
            if (result.status != StoreStatus::ok || !result.value.has_value() || result.value.value() != "value_" + std::to_string(i)) {
                return;
            }
        }
        all_ok = 1;
        };
    threads.clear();
    for (int i = 0; i < count; i++) {
        threads.emplace_back(read_work, std::ref(store), i, std::ref(all_ok[i]));
    }
    for (int i = 0; i < count; i++) {
        threads[i].join();
    }

    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(all_ok[i]);
    }
}

TEST_F(KVStoreTest, ConcurrentWritesThenRestart) {
    int count = 8;
    std::vector<std::thread> threads;
    std::vector<char> all_ok(count, 0);
    {
        KVStore store(path.string());

        auto write_work = [](KVStore& store, int thread_num, char& all_ok) {
            StoreStatus status;
            std::string sthread_num = std::to_string(thread_num);
            for (int i = 0; i < 1000; i++) {
                std::string key = "thread_" + sthread_num + "key" + std::to_string(i);
                status = store.put(key, std::to_string(thread_num));
                if (status != StoreStatus::ok) {
                    return;
                }
            }
            all_ok = 1;
            };
        for (int i = 0; i < count; i++) {
            threads.emplace_back(write_work, std::ref(store), i, std::ref(all_ok[i]));
        }

        for (int i = 0; i < count; i++) {
            threads[i].join();
        }
        for (int i = 0; i < count; i++) {
            ASSERT_TRUE(all_ok[i]);
        }
    }
    KVStore store(path.string());
    GetResult result;
    for (int i = 0; i < count; i++) {
        std::string sthread_num = std::to_string(i);
        for (int j = 0; j < 1000; j++) {
            std::string key = "thread_" + sthread_num + "key" + std::to_string(j);
            result = store.get(key);
            if (result.status != StoreStatus::ok || !result.value.has_value() || result.value.value() != sthread_num) {
                all_ok[i] = 0;
                break;
            }
        }
        if (!all_ok[i]) {
            break;
        }
    }

    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(all_ok[i]);
    }
}
TEST_F(KVStoreTest, CompactPreservesUpdatesAndDeletesAfterRestart) {
    {
        KVStore store(path.string());
        ASSERT_EQ(store.put("A", "old"), StoreStatus::ok);
        ASSERT_EQ(store.put("A", "new"), StoreStatus::ok);
        ASSERT_EQ(store.put("B", "deleted"), StoreStatus::ok);
        ASSERT_EQ(store.remove("B"), StoreStatus::ok);
        ASSERT_EQ(store.compact(), StoreStatus::ok);
    }
    KVStore reopened(path.string());
    ASSERT_EQ(reopened.get("A").value, std::optional<std::string>{"new"});
    ASSERT_EQ(reopened.get("B").status, StoreStatus::not_found);
}

TEST_F(KVStoreTest, WriteFailureBlocksFurtherMutations) {
    KVStore store(path.string());
    std::filesystem::create_directory(path);
    ASSERT_EQ(store.put("A", "1"), StoreStatus::io_error);
    std::filesystem::remove(path);
    EXPECT_EQ(store.put("A", "2"), StoreStatus::io_error);
    EXPECT_EQ(store.compact(), StoreStatus::io_error);
}

cmake_minimum_required(VERSION 3.20)

project(kv_store LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(GTest CONFIG REQUIRED)

add_library(kv_store_storage
	src/storage/KVStore.cpp
	src/storage/RecordCodec.cpp
)

target_include_directories(kv_store_storage PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(
	kv_store
	src/main.cpp
 "src/command/CommandParser.cpp" "src/core/Application.cpp")

add_executable(
	kv_store_tests
	"test/RecordCodecTests.cpp" "test/KVStoreTests.cpp"
)

target_link_libraries(kv_store PRIVATE kv_store_storage)

target_link_libraries(kv_store_tests PRIVATE
    kv_store_storage
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(kv_store_tests)
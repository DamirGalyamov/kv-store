#include "../src/storage/Status.hpp"
#include "../src/storage/KVStore.hpp"

#include <benchmark/benchmark.h>
#include <thread>
#include <filesystem>

static void BM_PUT(
	benchmark::State& state
)
{
	const std::filesystem::path path =
		"bench_test.db";

	std::filesystem::remove(path);
	{
		KVStore store("bench_test.db");
		int ikey = 0;
		std::string skey;
		for (auto _ : state) {
			state.PauseTiming();
			skey = std::to_string(ikey++);
			state.ResumeTiming();
			StoreStatus status = store.put(skey, "value");
			benchmark::DoNotOptimize(status);
		}
		state.SetItemsProcessed(
			state.iterations()
		);
	}
	std::filesystem::remove(path);
}

static void BM_GET_Single(
	benchmark::State& state
)
{
	const std::filesystem::path path =
		"bench_test.db";

	std::filesystem::remove(path);
	{
		KVStore store("bench_test.db");
		int ikey = 0;

		std::string skey;
		for (auto _ : state) {
			state.PauseTiming();
			skey = std::to_string(ikey++);
			StoreStatus status = store.put(skey, "value");
			state.ResumeTiming();
			auto result = store.get(skey);
			benchmark::DoNotOptimize(result);
		}
		state.SetItemsProcessed(
			state.iterations()
		);
	}
	std::filesystem::remove(path);
}

BENCHMARK(BM_PUT)->UseRealTime();
BENCHMARK(BM_GET_Single)->UseRealTime();
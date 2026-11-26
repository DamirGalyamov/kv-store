# KV Store
C++20 Windows command-line key/value store. Append-only binary log, CRC32 validation,
in-memory position index, shared/exclusive locking, deletion records and compaction.

## Build (Visual Studio 2022 and vcpkg)
```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\kv_store.exe
```
GoogleTest is required. Benchmarks are optional: configure with -DBUILD_BENCHMARKS=ON.

## Commands
```text
PUT "name" "value"
GET "name"
DELETE "name"
COMPACT
EXIT
```
The database is database.txt in the working directory (binary despite its extension).
Empty keys and values are rejected. Keys: at most 64 KiB; values: at most 16 MiB.

## Guarantees and limitations
Use a single KVStore instance per database file, in a single process. Methods on that
instance are synchronized. Concurrent independent instances/processes are unsupported.
A truncated final record is removed when reopening the database; other corruption fails
startup. After an append I/O error, further mutations are rejected until reopening.
Writes are not explicitly synchronized to physical storage: power-loss durability is
not guaranteed. Compaction temporarily loads live values into memory and blocks writers.

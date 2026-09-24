#pragma once

#include "RecordCodec.hpp"

#include <optional>
#include <string>

enum class StoreStatus
{
    ok,
    not_found,
    io_error,
    invalid_argument,
    corrupted_data,
    truncated_record
};

struct GetResult
{
    StoreStatus status;
    std::optional<std::string> value;
};

struct GetRecord
{
    StoreStatus status;
    std::optional<Record> record;
};

struct WriteResult
{
    StoreStatus status;
    std::streampos record_pos;
};
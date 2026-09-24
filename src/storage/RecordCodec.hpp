#pragma once

#include <optional>
#include <string>
#include <fstream>
#include <cstdint>
#include <cstddef>
#include <stdexcept>


enum class RecordType{
	put,
	remove
};

struct Record {
	RecordType type;
	std::string key;
	std::string value;
};

enum class CodecError
{
	truncated_record,
	checksum_mismatch,
	invalid_type,
	invalid_size,
	invalid_record,
	io_error
};

class RecordCodecException :
	public std::runtime_error
{
public:
	RecordCodecException(CodecError error, const std::string& message):
		std::runtime_error(message), error_(error) {

	}

	CodecError error() const
	{
		return error_;
	}

private:
	CodecError error_;
};

class RecordCodec {
public:
	std::optional<Record> read_one_record(std::istream& database)const;
	void write_one_record(std::ostream& database, const Record& record)const;

private:
	std::uint32_t read_uint32(std::istream& database, std::uint32_t* checksum)const;
	void read_string(std::istream& database, std::string& data, std::size_t data_size, std::uint32_t* checksum)const;
	std::uint32_t update_CRC32(std::uint32_t crc,const unsigned char* bytes, std::size_t length)const;

	static constexpr std::uint32_t max_key_size = 64 * 1024;
	static constexpr std::uint32_t max_value_size = 16 * 1024 * 1024;
	static constexpr std::uint32_t polynomial = 0xEDB88320;
	static constexpr std::uint32_t initial_crc = 0xFFFFFFFF;
};
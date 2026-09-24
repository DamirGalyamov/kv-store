#include "RecordCodec.hpp"
#include <vector>

std::optional<Record> RecordCodec::read_one_record(std::istream& database) const {
	Record record;
	std::uint32_t check_sum_of_record = initial_crc;
	std::uint32_t check_sum_from_file_record;
	std::uint8_t type;
	std::uint32_t key_size;
	std::uint32_t value_size;
	database.read(
		reinterpret_cast<char*>(&check_sum_from_file_record),
		sizeof(check_sum_from_file_record)
	);

	if (database.bad()) {
		throw RecordCodecException(CodecError::io_error,
			"I/O error while reading record");
	}

	if (database.gcount() == 0 && database.eof()) {
		return std::nullopt;
	}

	if (database.gcount() != sizeof(check_sum_from_file_record)) {
		throw RecordCodecException(CodecError::truncated_record, "Truncated record header");
	}
	database.read(
		reinterpret_cast<char*>(&type),
		sizeof(type)
	);
	if (!database) {
		if (database.bad()) {
			throw RecordCodecException(CodecError::io_error,
				"I/O error while reading record");
		}
		throw RecordCodecException(CodecError::truncated_record, "Unexpected end of record");
	}
	switch (type) {
	case 0: {
		record.type = RecordType::put;
		break;
	}
	case 1: {
		record.type = RecordType::remove;
		break;
	}
	default: {
		throw RecordCodecException(CodecError::invalid_type, "Unknown record type");
	}
	}
	check_sum_of_record = update_CRC32(check_sum_of_record, reinterpret_cast<const unsigned char*>(&type), sizeof(type));
	key_size = read_uint32(database, &check_sum_of_record);
	value_size = read_uint32(database, &check_sum_of_record);
	if (key_size > max_key_size || value_size > max_value_size) {
		throw RecordCodecException(CodecError::invalid_size, "Corrupted data");
	}
	if (record.type == RecordType::remove && value_size != 0) {
		throw RecordCodecException(CodecError::invalid_size,
			"DELETE record has non-zero value size");
	}
	read_string(database, record.key, key_size, &check_sum_of_record);
	if (record.type == RecordType::put) {
		read_string(database, record.value, value_size, &check_sum_of_record);
	}
	check_sum_of_record ^= initial_crc;
	if (check_sum_from_file_record != check_sum_of_record) {
		throw RecordCodecException(CodecError::checksum_mismatch, "Record checksum mismatch");
	}
	else {
		return record;
	}
	
}

void RecordCodec::write_one_record(std::ostream& database, const Record& record)const {
	std::uint32_t check_sum = initial_crc;
	std::uint32_t key_size = 0;
	std::uint32_t value_size = 0;
	std::uint8_t type;
	if (record.key.size() > max_key_size || record.value.size() > max_value_size) {
		throw RecordCodecException(CodecError::invalid_size, "Key or value size is too large");
	}
	key_size = static_cast<std::uint32_t>(record.key.size());
	if (record.type == RecordType::remove &&
		!record.value.empty()) {
		throw RecordCodecException(
			CodecError::invalid_record,
			"DELETE record cannt have not empty value"
		);
	}
	switch (record.type) {
	case RecordType::put:
		type = 0;
		value_size =
			static_cast<std::uint32_t>(
				record.value.size());
		break;

	case RecordType::remove:
		type = 1;
		value_size = 0;
		break;

	default:
		throw RecordCodecException(
			CodecError::invalid_type,
			"Unknown record type"
		);
	}
	check_sum = update_CRC32(check_sum, reinterpret_cast<const unsigned char*>(&type), sizeof(type));
	check_sum = update_CRC32(check_sum, reinterpret_cast<const unsigned char*>(&key_size), sizeof(key_size));
	check_sum = update_CRC32(check_sum, reinterpret_cast<const unsigned char*>(&value_size), sizeof(value_size));
	check_sum = update_CRC32(check_sum, reinterpret_cast<const unsigned char*>(record.key.data()), key_size);
	if (record.type == RecordType::put) {
		check_sum = update_CRC32(check_sum, reinterpret_cast<const unsigned char*>(record.value.data()), value_size);
	}
	check_sum ^= initial_crc;
	database.write(
		reinterpret_cast<const char*>(&check_sum),
		sizeof(check_sum)
	);
	if (!database) {
		throw RecordCodecException(CodecError::io_error, "Cannot write record chek summ");
	}
	database.write(
		reinterpret_cast<const char*>(&type),
		sizeof(type)
	);
	if (!database) {
		throw RecordCodecException(CodecError::io_error, "Cannot write record type");
	}
	database.write(
		reinterpret_cast<const char*>(&key_size),
		sizeof(key_size)
	);
	if (!database) {
		throw RecordCodecException(CodecError::io_error, "Cannot write record chek summ");
	}
	database.write(
		reinterpret_cast<const char*>(&value_size),
		sizeof(value_size)
	);
	if (!database) {
		throw RecordCodecException(CodecError::io_error, "Cannot write record value size");
	}
	database.write(
		reinterpret_cast<const char*>(record.key.data()),
		key_size
	);
	if (!database) {
		throw RecordCodecException(CodecError::io_error, "Cannot write record key");
	}

	if (record.type == RecordType::put) {
		database.write(
			reinterpret_cast<const char*>(record.value.data()),
			value_size
		);
		if (!database) {
			throw RecordCodecException(CodecError::io_error, "Cannot write record value");
		}
	}


}

std::uint32_t RecordCodec::update_CRC32(std::uint32_t crc, const unsigned char* bytes, std::size_t length)const {
	for (std::size_t byte = 0; byte < length; byte++) {
		crc ^= bytes[byte];
		for (int bit = 0; bit < 8; ++bit) {
			if (crc & 1u) {
				crc = (crc >> 1) ^ polynomial;
			}
			else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

std::uint32_t RecordCodec::read_uint32(std::istream& database,std::uint32_t* checksum = nullptr)const {
	std::uint32_t buffer;
	database.read(
		reinterpret_cast<char*>(&buffer),
		sizeof(buffer)
	);
	if (!database) {
		if (database.bad()) {
			throw RecordCodecException(CodecError::io_error,
				"I/O error while reading record");
		}
		throw RecordCodecException(CodecError::truncated_record, "Unexpected end of record");
	}
	if(checksum!=nullptr){
		*checksum = update_CRC32(*checksum,reinterpret_cast<const unsigned char*>(&buffer), sizeof(buffer));
	}
	return buffer;
}

void RecordCodec::read_string(std::istream& database, std::string& data, std::size_t data_size, std::uint32_t* checksum = nullptr)const {
	data.resize(data_size);
	database.read(
		data.data(),
		data_size
	);
	if (!database) {
		if (database.bad()) {
			throw RecordCodecException(CodecError::io_error,
				"I/O error while reading record");
		}
		throw RecordCodecException(CodecError::truncated_record, "Unexpected end of record");
	}
	if (checksum != nullptr) {
		*checksum = update_CRC32(*checksum, reinterpret_cast<const unsigned char*>(data.data()), data_size);
	}
}
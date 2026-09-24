#include "KVStore.hpp"

#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <optional>
#include <fstream>
#include <stdexcept>

KVStore::KVStore(const std::string& file_name) : file_name_(file_name), record_codec_()
{
	Init();
}

void KVStore::Init() {
	StoreStatus status = read_db();

	switch (status) {
	case StoreStatus::ok: return;
	case StoreStatus::io_error: throw std::runtime_error(
		"Failed to read storage file"
	);
	case StoreStatus::corrupted_data : throw std::runtime_error(
		"Storage file is corrupted"
	);
	}
}
//ready
StoreStatus KVStore::overwrite(std::unordered_map<std::string, std::string>& local_storage) {
	std::filesystem::path database_path = file_name_;
	std::filesystem::path copy_file_name = database_path;
	copy_file_name += L".tmp";
	std::ofstream file(copy_file_name, std::ios::binary | std::ios_base::trunc);
	std::unordered_map<std::string, std::streampos> local_kp_store;
	if (file.is_open()) {

		for (auto kvrecord : local_storage) {
			Record record{ RecordType::put, kvrecord.first, kvrecord.second };
			local_kp_store.emplace(kvrecord.first, file.tellp());
			try {
				record_codec_.write_one_record(file, record);
			}
			catch (const RecordCodecException& error) {
				return write_codec_error_to_status(error.error());
			}
		}
		file.flush();
		file.close();
		if (!file) {
			std::filesystem::remove(copy_file_name);
			return StoreStatus::io_error;
		}
		const auto flags =
			MOVEFILE_REPLACE_EXISTING |
			MOVEFILE_WRITE_THROUGH;
		if (!MoveFileExW(
			copy_file_name.c_str(),
			database_path.c_str(),
			flags))
		{
			return StoreStatus::io_error;
		}
		record_positions_in_file = local_kp_store;
		return StoreStatus::ok;
	}
	return StoreStatus::io_error;
}
//ready
WriteResult KVStore::write_at_the_end(const Record& record) {
	std::ofstream file(file_name_,std::ios::binary | std::ios::app);
	if (file.is_open()) {
		file.seekp(0, std::ios::end);
		if (!file) {
			return {
				StoreStatus::io_error,
				std::streampos{}
			};
		}
		std::streampos record_pos = file.tellp();
		try {
			record_codec_.write_one_record(file, record);
		}
		catch (const RecordCodecException& error) {
			return { write_codec_error_to_status(error.error()), std::streampos{} };
		}
		if (!file) {
			return { StoreStatus::io_error, std::streampos{} };
		}
		file.close();
		if (!file) {
			return { StoreStatus::io_error, std::streampos{} };
		}
		return { StoreStatus::ok, record_pos};
	}
	return { StoreStatus::io_error, std::streampos{} };
}

//ready
StoreStatus KVStore::read_db() {
	std::ifstream database(file_name_,
		std::ios::binary);
	if (database.is_open()) {
		std::optional<Record> record;
		std::streampos record_pos;
		try {
			record_pos = database.tellg();
			record = record_codec_.read_one_record(database);
			while (record) {
				if (record.value().type == RecordType::put) {
					record_positions_in_file[record.value().key] = record_pos;
				}
				else {
					auto it = record_positions_in_file.find(record.value().key);
					if (it != record_positions_in_file.end()) {
						record_positions_in_file.erase(it->first);
					}
				}
				record_pos = database.tellg();
				record = record_codec_.read_one_record(database);

			}
		}
		catch (const RecordCodecException& error) {
			StoreStatus status = read_codec_error_to_status(error.error());
			if (status == StoreStatus::truncated_record) {
				database.close();
				std::error_code ec;
				const auto new_size =
					static_cast<std::uintmax_t>(
						static_cast<std::streamoff>(record_pos)
						);
				std::filesystem::resize_file(
					file_name_,new_size,ec
				);
				if (ec) {
					return StoreStatus::io_error;
				}
				//залогировать
				return StoreStatus::ok;
			}
			return status;
		}
		return StoreStatus::ok;
	}
	if(!std::filesystem::exists(file_name_)) {
		return StoreStatus::ok;
	}
	return StoreStatus::io_error;
}
//ready
StoreStatus KVStore::read_db_for_local_storage(std::unordered_map<std::string, std::string>& local_storage) {
	std::ifstream database(file_name_,
		std::ios::binary);
	if (!database.is_open()) {
		return StoreStatus::io_error;
	}
	for (auto& rec : record_positions_in_file) {
		GetRecord record = read_current_record(database, rec.second);
		if (record.status != StoreStatus::ok) {
			return record.status;
		}
		local_storage[rec.first] = record.record.value().value;
	}
	return StoreStatus::ok;
}

StoreStatus KVStore::write_codec_error_to_status (
	const CodecError& error) const {
	switch (error) {
	case CodecError::io_error:
		return StoreStatus::io_error;
	case CodecError::invalid_record:
		return StoreStatus::invalid_argument;
	case CodecError::invalid_size:
		return StoreStatus::invalid_argument;
	default:
		return StoreStatus::corrupted_data;
	}
}

StoreStatus KVStore::read_codec_error_to_status(
	const CodecError& error) const {
	switch (error) {
	case CodecError::io_error:
		return StoreStatus::io_error;
	case CodecError::truncated_record: {
		return StoreStatus::truncated_record;
	}
	default:
		return StoreStatus::corrupted_data;
	}
}
//ready
GetRecord KVStore::read_current_record(std::ifstream& database, std::streampos record_pos) const {
	database.seekg(record_pos, std::ios::beg);
	if (!database) {
		return {
			StoreStatus::io_error,
			std::nullopt
		};
	}
	StoreStatus status = StoreStatus::ok;
	std::optional<Record> record;
	try {
		record = record_codec_.read_one_record(database);
	}
	catch (const RecordCodecException& error) {
		std::cout << error.what() << "\n";
		status = read_codec_error_to_status(error.error());
		if (status == StoreStatus::truncated_record) status = StoreStatus::corrupted_data;
		return { status, std::nullopt };
	}
	if (!record || record.value().type != RecordType::put) {
		return { StoreStatus::corrupted_data, std::nullopt };
	}
	return { status, record.value() };

}
//ready
GetResult KVStore::get(const std::string& key) const {
	if (key.empty()) {
		return { StoreStatus::invalid_argument, std::nullopt };
	}
	auto it = record_positions_in_file.find(key);
	if (it == record_positions_in_file.end()) {
		return { StoreStatus::not_found, std::nullopt };
	}
	std::ifstream database(file_name_,
		std::ios::binary);
	if (!database.is_open()) {
		return{
			StoreStatus::io_error,
			std::nullopt
		};
	}

	GetRecord record = read_current_record(
		database,
		it->second
	);
	if (record.status != StoreStatus::ok)
	{
		return { record.status, std::nullopt };
	}
	if(record.record.value().type != RecordType::put
		|| record.record.value().key != key) {
		return { StoreStatus::corrupted_data, std::nullopt };
	}
	return { record.status, record.record.value().value };

		
}
//in progress
StoreStatus KVStore::put(const std::string& key, const std::string& value) {
	if (key.empty() || value.empty()) {
		return StoreStatus::invalid_argument;
	}
	Record record{ RecordType::put, key, value };
	WriteResult wresult = write_at_the_end(record);
	if (wresult.status == StoreStatus::ok) {
		record_positions_in_file[key] = wresult.record_pos;
	}
	return wresult.status;
}

StoreStatus KVStore::remove(const std::string& key)
{
	if (key.empty()) {
		return StoreStatus::invalid_argument;
	}
	/*auto it = local_storage_.find(key);
	if (it != local_storage_.end()) {
		std::string text = "DELETE " + key + "\n";
		if (write_at_the_end(text) == StoreStatus::ok) {
			local_storage_.erase(key);
			return StoreStatus::ok;
		}
		else {
			
			return StoreStatus::io_error;
		}
	}
	else {
		return StoreStatus::not_found;
	}*/
	auto it = record_positions_in_file.find(key);
	if (it != record_positions_in_file.end()) {
		Record record{ RecordType::remove, key, "" };
		WriteResult wresult = write_at_the_end(record);
		if (wresult.status == StoreStatus::ok) {
			record_positions_in_file.erase(it->first);
		}
		return wresult.status;
	}
	else {
		return StoreStatus::not_found;
	}
}

StoreStatus KVStore::compact() 
{
	/*std::string data_base = "";
	for (const auto& pair : local_storage_) {
		data_base += "PUT " + pair.first + " " + pair.second + "\n";
	}
	if (overwrite(data_base) == StoreStatus::ok) {
		return StoreStatus::ok;
	}
	else {
		return StoreStatus::io_error;
	}*/
	std::unordered_map<std::string, std::string> local_storage;
	StoreStatus status;
	status = read_db_for_local_storage(local_storage);
	if (status == StoreStatus::ok) {
		status = overwrite(local_storage);
	}
	return status;

	//

}
#pragma once

#include "Status.hpp"
#include "RecordCodec.hpp"

#include <unordered_map>
#include <string>

class KVStore {
public:
	explicit KVStore(const std::string& file_name);

	GetResult get(const std::string& key) const;

	StoreStatus put(const std::string& key, const std::string& value);

	StoreStatus remove(const std::string& key);

	StoreStatus compact();

	//~KVStore();

private:

	WriteResult write_at_the_end(const Record&);

	StoreStatus overwrite(std::unordered_map<std::string, std::string>& local_storage);

	StoreStatus read_db();

	StoreStatus read_db_for_local_storage(std::unordered_map<std::string, std::string>& local_storage);

	StoreStatus write_codec_error_to_status(const CodecError& error) const;
	StoreStatus read_codec_error_to_status(const CodecError& error) const;

	GetRecord read_current_record(std::ifstream&, std::streampos)const;

	void Init();

	const std::string file_name_;
	std::unordered_map<std::string, std::string> local_storage_;
	std::unordered_map<std::string, std::streampos> record_positions_in_file;
	const RecordCodec record_codec_;
};
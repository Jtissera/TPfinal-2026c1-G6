#pragma once

#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

template <typename KeyType>
class BinaryArchiveFile
{
public:
    BinaryArchiveFile(const std::string &datPath,
                      const std::string &indexPath,
                      std::size_t recordSize,
                      std::size_t keyFieldSize);

    bool hasKey(const KeyType &key) const;
    std::optional<uint64_t> getOffset(const KeyType &key) const;
    uint64_t allocateSlot(const KeyType &key);

    void writeRecord(uint64_t offset, const void *data, std::size_t size);
    void appendToIndex(const KeyType &key, uint64_t offset);

    bool readRecord(uint64_t offset, void *dest, std::size_t size) const;
    std::vector<uint64_t> allOffsets() const;
    uint64_t recordCount() const;

private:
    void loadIndex();
    void writeKey(std::ofstream &idx, const KeyType &key) const;
    void readKey(std::ifstream &f, KeyType &key) const;

    std::string datPath_;
    std::string indexPath_;
    std::size_t recordSize_;
    std::size_t keyFieldSize_;

    mutable std::shared_mutex mutex_;
    std::unordered_map<KeyType, uint64_t> index_;
};
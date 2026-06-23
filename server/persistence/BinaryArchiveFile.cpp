#include "BinaryArchiveFile.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

template <typename KeyType>
BinaryArchiveFile<KeyType>::BinaryArchiveFile(const std::string &datPath,
                                              const std::string &indexPath,
                                              std::size_t recordSize,
                                              std::size_t keyFieldSize)
    : datPath_(datPath),
      indexPath_(indexPath),
      recordSize_(recordSize),
      keyFieldSize_(keyFieldSize)
{
    std::filesystem::path p(datPath_);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path());

    if (!std::filesystem::exists(datPath_))
    {
        std::ofstream f(datPath_, std::ios::binary);
    }
    if (!std::filesystem::exists(indexPath_))
    {
        std::ofstream f(indexPath_, std::ios::binary);
    }

    loadIndex();
}

template <typename KeyType>
void BinaryArchiveFile<KeyType>::writeKey(std::ofstream &idx, const KeyType &key) const
{
    if constexpr (std::is_same_v<KeyType, std::string>)
    {
        char buf[256] = {};
        std::strncpy(buf, key.c_str(), keyFieldSize_ - 1);
        idx.write(buf, static_cast<std::streamsize>(keyFieldSize_));
    }
    else
    {
        idx.write(reinterpret_cast<const char *>(&key), static_cast<std::streamsize>(keyFieldSize_));
    }
}

template <typename KeyType>
void BinaryArchiveFile<KeyType>::readKey(std::ifstream &f, KeyType &key) const
{
    if constexpr (std::is_same_v<KeyType, std::string>)
    {
        std::vector<char> buf(keyFieldSize_, '\0');
        f.read(buf.data(), static_cast<std::streamsize>(keyFieldSize_));
        key = std::string(buf.data(), strnlen(buf.data(), keyFieldSize_));
    }
    else
    {
        f.read(reinterpret_cast<char *>(&key), static_cast<std::streamsize>(keyFieldSize_));
    }
}

template <typename KeyType>
void BinaryArchiveFile<KeyType>::loadIndex()
{
    std::ifstream f(indexPath_, std::ios::binary);
    if (!f.is_open())
        return;

    KeyType key{};
    uint64_t offset = 0;

    while (true)
    {
        readKey(f, key);
        if (!f)
            break;
        f.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t));
        if (!f)
            break;
        index_[key] = offset;
    }
}

template <typename KeyType>
bool BinaryArchiveFile<KeyType>::hasKey(const KeyType &key) const
{
    std::shared_lock lock(mutex_);
    return index_.count(key) > 0;
}

template <typename KeyType>
std::optional<uint64_t> BinaryArchiveFile<KeyType>::getOffset(const KeyType &key) const
{
    std::shared_lock lock(mutex_);
    typename std::unordered_map<KeyType, uint64_t>::const_iterator it = index_.find(key);
    if (it == index_.end())
        return std::nullopt;
    return it->second;
}

template <typename KeyType>
uint64_t BinaryArchiveFile<KeyType>::allocateSlot(const KeyType &key)
{
    std::unique_lock lock(mutex_);
    uint64_t offset = static_cast<uint64_t>(index_.size()) * recordSize_;
    index_[key] = offset;
    return offset;
}

template <typename KeyType>
void BinaryArchiveFile<KeyType>::writeRecord(uint64_t offset, const void *data, std::size_t size)
{
    std::fstream dat(datPath_, std::ios::binary | std::ios::in | std::ios::out);
    dat.seekp(static_cast<std::streamoff>(offset));
    dat.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
    dat.flush();
}

template <typename KeyType>
void BinaryArchiveFile<KeyType>::appendToIndex(const KeyType &key, uint64_t offset)
{
    std::ofstream idx(indexPath_, std::ios::binary | std::ios::app);
    writeKey(idx, key);
    idx.write(reinterpret_cast<const char *>(&offset), sizeof(uint64_t));
    idx.flush();
}

template <typename KeyType>
bool BinaryArchiveFile<KeyType>::readRecord(uint64_t offset, void *dest, std::size_t size) const
{
    std::ifstream dat(datPath_, std::ios::binary);
    if (!dat.is_open())
        return false;
    dat.seekg(static_cast<std::streamoff>(offset));
    dat.read(reinterpret_cast<char *>(dest), static_cast<std::streamsize>(size));
    return dat.good();
}

template <typename KeyType>
std::vector<uint64_t> BinaryArchiveFile<KeyType>::allOffsets() const
{
    std::shared_lock lock(mutex_);
    std::vector<uint64_t> offsets;
    offsets.reserve(index_.size());
    for (typename std::unordered_map<KeyType, uint64_t>::const_iterator it = index_.begin();
         it != index_.end(); ++it)
    {
        offsets.push_back(it->second);
    }
    return offsets;
}

template <typename KeyType>
uint64_t BinaryArchiveFile<KeyType>::recordCount() const
{
    std::shared_lock lock(mutex_);
    return static_cast<uint64_t>(index_.size());
}

template class BinaryArchiveFile<std::string>;
template class BinaryArchiveFile<uint32_t>;
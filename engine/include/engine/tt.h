#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace la
{

// EntryType must be POD and it must have a public `hash` member.
template <typename EntryType, std::size_t max_entries>
class TT
{
static_assert(std::is_pod<EntryType>::value);
public:
  TT();
  bool probe(std::uint64_t hash, EntryType** entry);

private:
  std::vector<EntryType> entries_;
};

template <typename EntryType, std::size_t max_entries>
TT<EntryType, max_entries>::TT()
{
  entries_.resize(max_entries);
  std::memset(entries_.data(), 0, max_entries * sizeof(EntryType));
}

template <typename EntryType, std::size_t max_entries>
bool TT<EntryType, max_entries>::probe(std::uint64_t hash, EntryType** entry)
{
  *entry = &entries_[hash % max_entries];
  return (*entry)->hash == hash;
}

}

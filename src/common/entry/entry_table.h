#pragma once
#include <common/entry/entry_song.h>
#include <common/hash.h>
#include <common/types.h>

class EntryFolderTable : public EntryFolderRegular
{
public:
    EntryFolderTable() = delete;
    EntryFolderTable(StringContentView name, size_t index) : EntryFolderRegular(HashMD5{}, "", name, ""), _index(index)
    {
        _type = eEntryType::CUSTOM_FOLDER;
    }

    [[nodiscard]] size_t getIndex() const { return _index; }

private:
    size_t _index;
};

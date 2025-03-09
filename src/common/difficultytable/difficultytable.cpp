#include "difficultytable.h"

#include <common/entry/entry.h>
#include <common/utils.h>

#include <algorithm>
#include <climits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::string> DifficultyTable::getLevelList() const
{
    std::vector<std::string> levelList;
    levelList.reserve(entries.size());
    for (const auto& entry : entries)
        levelList.push_back(entry.first);
    auto rank = [this](const std::string& key) -> int {
        if (auto it = _levelOrder.find(key); it != _levelOrder.end())
            return it->second;
        return toInt(key, INT_MAX);
    };
    std::ranges::sort(levelList, {}, rank);
    return levelList;
}

std::vector<std::shared_ptr<EntryBase>> DifficultyTable::getEntryList(const std::string& level)
{
    if (auto it = entries.find(level); it != entries.end())
        return it->second;
    return {};
}

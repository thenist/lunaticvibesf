#include "entry_song.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

std::shared_ptr<ChartFormatBase> EntryFolderSong::getChart(size_t idx)
{
    idx %= getContentsCount();
    return charts[idx];
}
std::shared_ptr<ChartFormatBase> EntryFolderSong::getCurrentChart()
{
    if (charts.empty())
        return nullptr;
    idx %= getContentsCount();
    return charts[idx];
}

size_t EntryFolderSong::incCurrentChart()
{
    idx++;
    idx %= getContentsCount();
    return idx;
}

void EntryFolderSong::pushChart(std::shared_ptr<ChartFormatBase> c)
{
    charts.push_back(c);
    chartMap[c->gamemode][c->difficulty].push_back(std::move(c));
}

const std::vector<std::shared_ptr<ChartFormatBase>>& EntryFolderSong::getDifficultyList(int gamemode,
                                                                                        unsigned difficulty) const
{
    if (auto diffMap = chartMap.find(gamemode); diffMap != chartMap.end())
        if (auto diffList = diffMap->second.find(difficulty); diffList != diffMap->second.end())
            return diffList->second;
    static const std::vector<std::shared_ptr<ChartFormatBase>> emptyList;
    return emptyList;
}

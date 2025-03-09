#include "entry_random_song.h"

#include <sstream>
#include <string>
#include <utility>

#include <common/entry/entry.h>
#include <common/hash.h>

lunaticvibes::EntryRandomChart::EntryRandomChart(std::string name_, std::string name2_, Filter filter) : _filter(filter)
{
    _type = eEntryType::RANDOM_CHART;
    _name = std::move(name_);
    _name2 = std::move(name2_);

    std::stringstream ss;
    ss << static_cast<int>(filter);
    ss << _name;
    ss << _name2;
    this->md5 = ::md5(ss.str());
};

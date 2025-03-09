#include "config.h"

#include <common/log.h>
#include <common/meta.h>
#include <common/types.h>
#include <common/u8.h>

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <ios>
#include <string>

vConfig::vConfig()
{
    // placeholder, do nothing
}

vConfig::vConfig(const StringPath& file)
{
    _path = file;
}

vConfig::vConfig(const char* file)
{
    _path = file;
}

vConfig::vConfig(const std::string& profile, const char* file)
{
    _path = GAMEDATA_PATH;
    _path /= "profile";
    _path /= profile;
    _path /= file;
}

vConfig::vConfig(const std::string& profile, const StringPath& file)
{
    _path = GAMEDATA_PATH;
    _path /= "profile";
    _path /= profile;
    _path /= file;
}

void vConfig::load()
{
    setDefaults();
    try
    {
        for (const auto& node : YAML::LoadFile(lunaticvibes::u8str(_path)))
        {
            _yaml[node.first.as<std::string>()] = node.second;
        }
    }
    catch (YAML::BadFile&)
    {
        LOG_WARNING << "[Config] Bad file: " << _path;
    }
}

void vConfig::save()
{
    std::ofstream fout(_path, std::ios_base::trunc);
    fout << _yaml;
}

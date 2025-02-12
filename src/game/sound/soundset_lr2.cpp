#include "soundset_lr2.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include <string_view>
#include <utility>

#include <common/encoding.h>
#include <common/log.h>
#include <common/str_utils.h>
#include <common/u8.h>
#include <common/utils.h>
#include <config/config_mgr.h>
#include <game/scene/scene_customize.h>

namespace r = std::ranges;

SoundSetLR2::SoundSetLR2() : SoundSetLR2(std::mt19937{std::random_device{}()})
{
    _type = eSoundSetType::LR2;
}

SoundSetLR2::SoundSetLR2(Path p) : SoundSetLR2()
{
    loadCSV(std::move(p));
}

SoundSetLR2::SoundSetLR2(std::mt19937 gen) : _gen(gen) {}

void SoundSetLR2::loadCSV(Path p)
{
    if (filePath.empty())
        filePath = p;

    auto srcLineNumberParent = csvLineNumber;
    csvLineNumber = 0;

    p = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."), p);

    std::ifstream ifsFile(p, std::ios::binary);
    if (!ifsFile.is_open())
    {
        LOG_ERROR << "[SoundSet] File not found: " << p;
        csvLineNumber = srcLineNumberParent;
        return;
    }

    // copy the whole file into ram, once for all
    std::stringstream csvFile;
    csvFile << ifsFile.rdbuf();
    csvFile.sync();
    ifsFile.close();

    auto encoding = getFileEncoding(csvFile);

    LOG_INFO << "[SoundSet] File (" << getFileEncodingName(encoding) << "): " << p;

    std::vector<StringContent> tokenBuf;
    tokenBuf.reserve(32);

    for (std::string rawUTF8, raw_; std::getline(csvFile, raw_);)
    {
        ++csvLineNumber;
        to_utf8(raw_, encoding, rawUTF8);
        lunaticvibes::trim_in_place(rawUTF8);
        lunaticvibes::split(rawUTF8, ',', tokenBuf);
        parseHeader(tokenBuf);
    }

    // load skin customization from profile
    Path pCustomize = ConfigMgr::Profile()->getPath() / "customize" / SceneCustomize::getConfigFileName(getFilePath());
    try
    {
        std::map<StringContent, StringContent> opFileMap;
        for (const auto& node : YAML::LoadFile(lunaticvibes::u8str(pCustomize)))
        {
            auto key = node.first.as<std::string>();
            if (key.substr(0, 5) == "FILE_")
            {
                setCustomFileOptionForBodyParsing(key.substr(5), node.second.as<std::string>());
            }
            else
            {
                LOG_WARNING << "[SoundSetLR2] Unknown config option '" << key << "' ignored";
            }
        }
    }
    catch (YAML::BadFile&)
    {
        LOG_WARNING << "[Skin] Bad customize config file: " << pCustomize;
    }

    csvFile.clear();
    csvFile.seekg(0);
    csvLineNumber = 0;
    for (std::string rawUTF8, raw_; std::getline(csvFile, raw_);)
    {
        ++csvLineNumber;
        to_utf8(raw_, encoding, rawUTF8);
        lunaticvibes::trim_in_place(rawUTF8);
        lunaticvibes::split(rawUTF8, ',', tokenBuf);
        parseBody(tokenBuf);
    }

    LOG_DEBUG << "[SoundSet] File: " << p << "(Line " << csvLineNumber << "): loading finished";
}

bool SoundSetLR2::parseHeader(const std::vector<StringContent>& tokens)
{
    if (tokens.empty())
        return false;

    StringContentView key = tokens[0];

    if (lunaticvibes::iequals(key, "#INFORMATION"))
    {
        if (tokens.size() <= 1)
        {
            LOG_DEBUG << "[SoundSetLR2] Missing #INFORMATION fields";
            return false;
        }

        if (tokens[1] != "10")
        {
            LOG_DEBUG << "[SoundSetLR2] Invalid #INFORMATION type";
            return false;
        }
        if (tokens.size() > 2)
            name = tokens[2];
        if (tokens.size() > 3)
            maker = tokens[3];
        if (tokens.size() > 4)
            thumbnailPath = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."), lunaticvibes::trim(tokens[4]));

        return true;
    }
    if (lunaticvibes::iequals(key, "#CUSTOMFILE"))
    {
        if (tokens.size() < 4)
        {
            LOG_DEBUG << "[SoundSetLR2] Missing #CUSTOMFILE fields";
            return false;
        }

        const auto& title = tokens[1];
        const auto& path = tokens[2];
        Path pathf = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."), path);
        const auto& def = tokens[3];

        LOG_DEBUG << "[SoundSet] " << csvLineNumber << ": Loaded Custom file " << title << ": " << pathf;

        CustomFile c;
        c.title = title;
        c.filepath = lunaticvibes::u8str(pathf);
        for (auto& p : findFiles(pathf))
            c.label.push_back(lunaticvibes::u8str(p.filename()));

        r::sort(c.label);
        c.label.emplace_back("RANDOM");
        customizeRandom.push_back(
            c.label.size() < 2 ? 0 : std::uniform_int_distribution<size_t>{0, c.label.size() - 2}(_gen));

        c.defIdx = 0;
        for (size_t i = 0; i < c.label.size(); ++i)
        {
            if (c.label[i] == def)
            {
                c.defIdx = i;
                break;
            }
        }
        c.value = c.defIdx;
        customfiles.push_back(std::move(c));

        return true;
    }
    return false;
}

bool SoundSetLR2::parseBody(const std::vector<StringContent>& tokens)
{
    if (tokens.size() < 2)
        return false;

    StringContentView key = tokens[0];

    static const std::set<std::string> soundKeys = {
        "#SELECT",     "#DECIDE",      "#EXSELECT",      "#EXDECIDE",   "#FOLDER_OPEN", "#FOLDER_CLOSE",
        "#PANEL_OPEN", "#PANEL_CLOSE", "#OPTION_CHANGE", "#DIFFICULTY", "#SCREENSHOT",  "#CLEAR",
        "#FAIL",       "#STOP",        "#MINE",          "#SCRATCH",    "#COURSECLEAR", "#COURSEFAIL",
    };
    for (auto& k : soundKeys)
    {
        if (lunaticvibes::iequals(k, key))
        {
            return loadPath(k, tokens[1]);
        }
    }
    return false;
}

bool SoundSetLR2::loadPath(const std::string& key, const std::string_view rawpath)
{
    if (auto it = soundFilePath.find(key); it != soundFilePath.end())
    {
        LOG_DEBUG << "[SoundSetLR2] Ignoring duplicate key " << key;
        return false;
    }

    Path path = convertLR2Path(ConfigMgr::get('E', cfg::E_LR2PATH, "."), rawpath);
    const std::string pathU8Str = lunaticvibes::u8str(path);
    const std::string_view pathU8StrView{pathU8Str};
    if (pathU8StrView.find(u8'*') != pathU8StrView.npos)
    {
        bool customFileFound = false;

        // Check if the wildcard path is specified by custom settings
        for (size_t idx = 0; idx < customfiles.size(); ++idx)
        {
            const auto& cf = customfiles[idx];
            if (cf.filepath == pathU8StrView.substr(0, cf.filepath.length()))
            {
                const size_t value = (cf.label[cf.value] == "RANDOM") ? customizeRandom[idx] : cf.value;
                // Last entry is always 'RANDOM'.
                if (value == cf.label.size() - 1)
                {
                    LOG_DEBUG << "[SoundSetLR2] Random rolled 'RANDOM' (no files found?)";
                    break;
                }

                std::string pathFile = pathU8Str;
                lunaticvibes::replace_all(pathFile, "*", cf.label[value]);
                soundFilePath.insert_or_assign(key, pathFile);
                LOG_DEBUG << "[Skin] " << csvLineNumber << ": Added " << key << ": " << pathFile;

                customFileFound = true;
                break;
            }
        }

        if (!customFileFound)
        {
            // Or, randomly choose a file
            auto ls = findFiles(path);
            if (ls.empty())
            {
                soundFilePath[key] = Path();
                LOG_DEBUG << "[Skin] " << csvLineNumber << ": Added random " << key << ": " << "(placeholder)";
            }
            else
            {
                // Sort for determinism in tests.
                r::sort(ls);
                const size_t ranidx = std::uniform_int_distribution<size_t>{0, ls.size() - 1}(_gen);
                const Path& soundPath = ls[ranidx];
                soundFilePath[key] = soundPath;
                LOG_DEBUG << "[Skin] " << csvLineNumber << ": Added random " << key << ": " << soundPath;
            }
        }
    }
    else
    {
        // Normal path
        soundFilePath[key] = path;
        LOG_DEBUG << "[Skin] " << csvLineNumber << ": Added " << key << ": " << path;
    }

    // Good even if it's empty, as that's not the problem with CSVs, it's missing files.
    return soundFilePath.contains(key);
}

static Path getPathOrDefault(const std::map<std::string, Path>& soundFilePath, const std::string& key)
{
    if (auto it = soundFilePath.find(key); it != soundFilePath.end())
        return it->second;
    return {};
}

Path SoundSetLR2::getPathBGMSelect() const
{
    return getPathOrDefault(soundFilePath, "#SELECT");
}

Path SoundSetLR2::getPathBGMDecide() const
{
    return getPathOrDefault(soundFilePath, "#DECIDE");
}

Path SoundSetLR2::getPathSoundOpenFolder() const
{
    return getPathOrDefault(soundFilePath, "#FOLDER_OPEN");
}

Path SoundSetLR2::getPathSoundCloseFolder() const
{
    return getPathOrDefault(soundFilePath, "#FOLDER_CLOSE");
}

Path SoundSetLR2::getPathSoundOpenPanel() const
{
    return getPathOrDefault(soundFilePath, "#PANEL_OPEN");
}

Path SoundSetLR2::getPathSoundClosePanel() const
{
    return getPathOrDefault(soundFilePath, "#PANEL_CLOSE");
}

Path SoundSetLR2::getPathSoundOptionChange() const
{
    return getPathOrDefault(soundFilePath, "#OPTION_CHANGE");
}

Path SoundSetLR2::getPathSoundDifficultyChange() const
{
    return getPathOrDefault(soundFilePath, "#DIFFICULTY");
}

Path SoundSetLR2::getPathSoundScreenshot() const
{
    return getPathOrDefault(soundFilePath, "#SCREENSHOT");
}

Path SoundSetLR2::getPathBGMResultClear() const
{
    return getPathOrDefault(soundFilePath, "#CLEAR");
}

Path SoundSetLR2::getPathBGMResultFailed() const
{
    return getPathOrDefault(soundFilePath, "#FAIL");
}

Path SoundSetLR2::getPathSoundFailed() const
{
    return getPathOrDefault(soundFilePath, "#STOP");
}

Path SoundSetLR2::getPathSoundLandmine() const
{
    return getPathOrDefault(soundFilePath, "#MINE");
}

Path SoundSetLR2::getPathSoundScratch() const
{
    return getPathOrDefault(soundFilePath, "#SCRATCH");
}

Path SoundSetLR2::getPathBGMCourseClear() const
{
    return getPathOrDefault(soundFilePath, "#COURSECLEAR");
}

Path SoundSetLR2::getPathBGMCourseFailed() const
{
    return getPathOrDefault(soundFilePath, "#COURSEFAIL");
}

size_t SoundSetLR2::getCustomizeOptionCount() const
{
    return customfiles.size();
}

SkinBase::CustomizeOption SoundSetLR2::getCustomizeOptionInfo(size_t idx) const
{
    SkinBase::CustomizeOption ret;
    const auto& op = customfiles[idx];

    ret.id = 0;
    ret.internalName = "FILE_";
    ret.internalName += op.title;
    ret.displayName = op.title;
    ret.entries = op.label;
    ret.defaultEntry = op.defIdx;

    return ret;
}

StringPath SoundSetLR2::getFilePath() const
{
    return filePath.is_absolute() ? filePath : filePath.relative_path();
}

StringPath SoundSetLR2::getThumbnailPath() const
{
    return thumbnailPath.is_absolute() ? thumbnailPath : thumbnailPath.relative_path();
}

bool SoundSetLR2::setCustomFileOptionForBodyParsing(const std::string_view title, const std::string_view value)
{
    for (auto& customFile : customfiles)
    {
        if (customFile.title == title)
        {
            if (const auto entry = r::find(customFile.label, value); entry != customFile.label.end())
            {
                customFile.value = std::distance(customFile.label.begin(), entry);
                return true;
            }
            LOG_WARNING << "[SoundSetLR2] Config value for option '" << title << "' '" << value << "' is not available";
            return false;
        }
    }
    LOG_WARNING << "[SoundSetLR2] Config option '" << title << "' not found";
    return false;
};

#include "chartformat.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>

#include <common/chartformat/chartformat_bms.h>
#include <common/encoding.h>
#include <common/log.h>
#include <common/u8.h>
#include <common/utils.h>

namespace r = std::ranges;

eChartFormat analyzeChartType(const Path& p)
{
    if (!p.has_extension())
        return eChartFormat::UNKNOWN;

    eChartFormat fmt = eChartFormat::UNKNOWN;

    auto extension = lunaticvibes::u8str(p.extension());
    if (extension.length() == 4)
    {
        if (lunaticvibes::iequals(extension, ".bms") || lunaticvibes::iequals(extension, ".bme") ||
            lunaticvibes::iequals(extension, ".bml") || lunaticvibes::iequals(extension, ".pms"))
            fmt = eChartFormat::BMS;
    }
    else if (extension.length() == 6)
    {
        if (lunaticvibes::iequals(extension, ".bmson"))
            fmt = eChartFormat::BMSON;
    }

    return fmt;
}

std::shared_ptr<ChartFormatBase> ChartFormatBase::createFromFile(const Path& path, uint64_t randomSeed)
{
    Path filePath = fs::absolute(path);
    std::ifstream fs{filePath};
    if (fs.fail())
    {
        LOG_WARNING << "[Chart] File invalid: " << filePath;
        return nullptr;
    }

    switch (analyzeChartType(path))
    {
    case eChartFormat::BMS:
        return std::static_pointer_cast<ChartFormatBase>(std::make_shared<ChartFormatBMS>(filePath, randomSeed));

    case eChartFormat::UNKNOWN: LOG_WARNING << "[Chart] File type unknown: " << filePath; return nullptr;

    default: LOG_WARNING << "[Chart] File type unsupported: " << filePath; return nullptr;
    }
}

std::string ChartFormatBase::formatReadmeText(const std::vector<std::pair<std::string, std::string>>& files)
{
    std::string out;
    // 1-indexed.
    size_t file_index{1};
    for (const auto& [file_name, text] : files)
    {
        if (files.size() > 1)
        {
            out += std::to_string(file_index);
            out += '/';
            out += std::to_string(files.size());
            out += ' ';
        }
        // NOTE: LR2 only shows file name if there are more than one file. We don't. :)
        out += file_name;
        out += "\n\n";
        out += text;
        out += '\n';
        ++file_index;
    }
    return out;
}

bool ChartFormatBase::checkHasReadme() const
try
{
    for (const auto& entry : fs::directory_iterator(getDirectory()))
    {
        if (lunaticvibes::iequals(lunaticvibes::s(entry.path().extension().u8string()), ".txt"))
            return true;
    }
    return false;
}
catch (const fs::filesystem_error& e)
{
    LOG_ERROR << "[Chart] checkHasReadme() Filesystem error: " << e.what();
    return false;
}

std::vector<std::pair<std::string, std::string>> ChartFormatBase::getReadmeFiles() const
try
{
    using Pair = std::pair<std::string, std::string>;
    std::vector<Pair> out;
    out.reserve(_comments.size());
    for (const auto& comment : _comments)
        out.emplace_back("", comment);
    std::string line;
    for (const auto& entry : fs::directory_iterator(getDirectory()))
    {
        const auto& file = entry.path();
        if (!lunaticvibes::iequals(lunaticvibes::s(file.extension().u8string()), ".txt"))
            continue;
        std::ifstream ifs{file};
        if (ifs.fail())
        {
            LOG_ERROR << "[Chart] Failed to open file " << file;
            continue;
        }
        auto encoding = getFileEncoding(ifs);
        std::string utf8_text;
        for (std::string line_; std::getline(ifs, line_);)
        {
            lunaticvibes::trim_in_place(line_);
            lunaticvibes::to_utf8(line_, encoding, line);
            utf8_text += line;
            utf8_text += '\n';
        }
        out.emplace_back(lunaticvibes::u8str(file.filename()), std::move(utf8_text));
    }
    r::sort(out, {}, &Pair::first);
    return out;
}
catch (const fs::filesystem_error& e)
{
    LOG_ERROR << "[Chart] getReadmeFiles() Filesystem error: " << e.what();
    return {};
}

Path ChartFormatBase::getDirectory() const
{
    return absolutePath.parent_path();
}

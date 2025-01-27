#include "lr2crs.h"

#include <exception>
#include <fstream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <common/encoding.h>
#include <common/hash.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <common/u8.h>
#include <common/utils.h>

static std::stringstream readFileIntoStringStream(const Path& filePath)
{
    std::ifstream ifs(filePath);
    if (ifs.fail())
    {
        LOG_WARNING << "[lr2crs] " << filePath << " File ERROR";
        return {};
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    ifs.close();
    return ss;
}

static std::stringstream readIntoUtf8StringStream(const Path& filePath)
{
    auto ss = readFileIntoStringStream(filePath);
    auto encoding = getFileEncoding(ss);
    std::stringstream ssUTF8;
    for (std::string lineBuf; std::getline(ss, lineBuf);)
        ssUTF8 << lunaticvibes::trim(to_utf8(lineBuf, encoding));
    return ssUTF8;
}

CourseLr2crs::CourseLr2crs(const Path& filePath)
    : CourseLr2crs(lunaticvibes::s(filePath.filename().u8string()), getFileLastWriteTime(filePath),
                   readIntoUtf8StringStream(filePath))
{
}

CourseLr2crs::CourseLr2crs(std::string_view source, long long addTime, std::stringstream ssUTF8) : addTime(addTime)
{
    // .lr2crs is a standard xml file. A file may contain multiple courses.
    // parse as XML
    try
    {
        namespace pt = boost::property_tree;
        pt::ptree tree;
        pt::read_xml(ssUTF8, tree);
        for (auto& course : tree.get_child("courselist"))
        {
            Course& c = courses.emplace_back();
            for (auto& [name, value] : course.second)
            {
                // NOTE: LR2 parses fields case-sensitively.
                if (name == "title")
                    c.title = value.data();
                else if (name == "line")
                    c.line = toInt(value.data());
                else if (name == "type")
                    c.type = decltype(c.type)(toInt(value.data()));
                else if (name == "hash")
                {
                    // first 32 characters are course metadata, of which we didn't make use; the 10th char is course
                    // type, the last 4 are assumed to be uploader ID after that, each 32 chars indicates a chart MD5
                    const std::string& hash = value.data();
                    c.hashTop = hash.substr(0, 32);
                    for (size_t count = 1; count < hash.size() / 32; ++count)
                        c.chartHash.emplace_back(hash.substr(count * 32, 32));
                }
                else
                {
                    LOG_WARNING << "[CourseLr2crs] Unknown field type " << name;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_WARNING << "[CourseLr2crs] Exception while parsing courses: " << e.what();
        courses.clear();
    }

    LOG_DEBUG << "[CourseLr2crs] Loaded " << courses.size() << " courses from " << source;
}

HashMD5 CourseLr2crs::Course::getCourseHash() const
{
    // NOTE: do *not* change this, or old Lunatic Vibes course scores in the DB will get broken.

    std::stringstream ss;
    ss << (int)type;
    ss << chartHash.size();
    for (auto& c : chartHash)
        ss << c.hexdigest();
    return md5(ss.str());
}

#include "i18n.h"

#include <fstream>
#include <sstream>

#include <common/meta.h>
#include <common/str_utils.h>
#include <common/u8.h>
#include <common/utils.h>

std::vector<i18n> i18n::languages;
size_t i18n::currentLanguage = 0;

i18n::i18n(const Path& translationFile)
{
    if (translationFile.stem() == "zh-cn")
    {
        type = Languages::ZHCN;
    }

    std::ifstream ifsFile(translationFile);
    std::stringstream ss;
    ss << ifsFile.rdbuf();
    ss.sync();
    ifsFile.close();

    // File is in UTF-8 encoding, no need to convert

    size_t i = 0;
    for (std::string line; std::getline(ss, line);)
    {
        if (i >= i18n_TEXT_COUNT)
            break;

        lunaticvibes::replace_all(line, "\\\\n", "\n");

        text[i] = line;
        i++;
    }
}

void i18n::init()
{
    // english first..
    Path en = Path(GAMEDATA_PATH) / "resources" / "translations" / "en.txt";
    languages.push_back(i18n(en));

    // other languages
    for (auto& f : fs::directory_iterator(Path(GAMEDATA_PATH) / "resources" / "translations"))
    {
        if (fs::equivalent(f.path(), en))
            continue;

        if (lunaticvibes::iequals(lunaticvibes::s(f.path().extension().u8string()), ".txt"))
        {
            languages.push_back(i18n(f.path()));
        }
    }
}

std::vector<std::string> i18n::getLanguageList()
{
    std::vector<std::string> res;
    res.reserve(languages.size());
    for (auto& l : languages)
    {
        res.push_back(l.text[i18nText::LANGUAGE_NAME]);
    }
    return res;
}

void i18n::setLanguage(size_t index)
{
    currentLanguage = index >= languages.size() ? 0 : index;
}

void i18n::setLanguage(const std::string& name)
{
    for (size_t i = 0; i < languages.size(); ++i)
    {
        if (languages[i].text[i18nText::LANGUAGE_NAME] == name)
        {
            currentLanguage = i;
            break;
        }
    }
}

const std::string& i18n::s(size_t index)
{
    static std::string emptyString;
    if (index >= i18n_TEXT_COUNT)
        return emptyString;

    return languages[currentLanguage].text[index];
}

const char* i18n::c(size_t index)
{
    return s(index).c_str();
}

Languages i18n::getCurrentLanguage()
{
    return languages[currentLanguage].type;
}

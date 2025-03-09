#include <common/types.h>

#include <common/log.h>

#include <ostream>

std::ostream& operator<<(std::ostream& os, const SkinType& mode)
{
    switch (mode)
    {
    case SkinType::EXIT: return os << "EXIT";
    case SkinType::TITLE: return os << "TITLE";
    case SkinType::MUSIC_SELECT: return os << "MUSIC_SELECT";
    case SkinType::DECIDE: return os << "DECIDE";
    case SkinType::THEME_SELECT: return os << "THEME_SELECT";
    case SkinType::SOUNDSET: return os << "SOUNDSET";
    case SkinType::KEY_CONFIG: return os << "KEY_CONFIG";
    case SkinType::PLAY5: return os << "PLAY5";
    case SkinType::PLAY5_2: return os << "PLAY5_2";
    case SkinType::PLAY7: return os << "PLAY7";
    case SkinType::PLAY7_2: return os << "PLAY7_2";
    case SkinType::PLAY9: return os << "PLAY9";
    case SkinType::PLAY9_2: return os << "PLAY9_2";
    case SkinType::PLAY10: return os << "PLAY10";
    case SkinType::PLAY14: return os << "PLAY14";
    case SkinType::RESULT: return os << "RESULT";
    case SkinType::COURSE_RESULT: return os << "COURSE_RESULT";
    case SkinType::RETRY_TRANS: return os << "RETRY_TRANS";
    case SkinType::COURSE_TRANS: return os << "COURSE_TRANS";
    case SkinType::EXIT_TRANS: return os << "EXIT_TRANS";
    case SkinType::PRE_SELECT: return os << "PRE_SELECT";
    case SkinType::TMPL: return os << "TMPL";
    case SkinType::TEST: return os << "TEST";
    case SkinType::MODE_COUNT: return os << "MODE_COUNT";
    }
    LOG_ERROR << "Invalid 'mode': " << static_cast<int>(mode);
    return os << "(invalid 'mode')";
}

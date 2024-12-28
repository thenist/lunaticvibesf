#include "log.h"

#include <memory>
#include <ostream>

#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/IAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>

#include <common/assert.h>
#include <common/types.h>

#ifdef _WIN32

#include <common/in_test_mode.h>

#include <plog/Appenders/DebugOutputAppender.h>

static bool have_console_window()
{
    return lunaticvibes::in_test_mode();
}

#endif // _WIN32

namespace plog
{
class TxtFormatterFileLine
{
public:
    static util::nstring header() { return {}; }

    static util::nstring format(const Record& record)
    {
        tm t;
        util::localtime_s(&t, &record.getTime().time);

        util::nostringstream ss;
        ss << t.tm_year + 1900 << "-" << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mon + 1 << PLOG_NSTR("-")
           << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mday << PLOG_NSTR(" ");
        ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":")
           << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0'))
           << std::setw(2) << t.tm_sec << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3)
           << static_cast<int>(record.getTime().millitm) << PLOG_NSTR(" ");
        ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity())
           << PLOG_NSTR(" ");
        ss << PLOG_NSTR("[") << record.getTid() << PLOG_NSTR("] ");
        ss << PLOG_NSTR("[") << fs::path(record.getFile()).filename() << PLOG_NSTR("@") << record.getLine()
           << PLOG_NSTR("] ");
        ss << record.getMessage() << PLOG_NSTR("\n");

        return ss.str();
    }
};
} // namespace plog

static std::shared_ptr<plog::IAppender> pConsoleAppender;
static std::shared_ptr<plog::IAppender> pTxtAppender;

namespace lunaticvibes
{

void InitLogger(const char* logFileName)
{
#ifdef _WIN32
    if (have_console_window())
        pConsoleAppender = std::make_shared<plog::ColorConsoleAppender<plog::TxtFormatterFileLine>>();
    else
        pConsoleAppender = std::make_shared<plog::DebugOutputAppender<plog::TxtFormatterFileLine>>();
#else
    pConsoleAppender = std::make_shared<plog::ColorConsoleAppender<plog::TxtFormatterFileLine>>();
#endif // WIN32
    plog::init(plog::info, pConsoleAppender.get());

    if (logFileName != nullptr)
    {
        pTxtAppender = std::make_shared<plog::RollingFileAppender<plog::TxtFormatterFileLine>>(logFileName, 1000000, 5);
        plog::get()->addAppender(pTxtAppender.get());
    }
}

static plog::Severity plogSeverityFromLogLevel(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::Fatal: return plog::fatal;
    case LogLevel::Error: return plog::error;
    case LogLevel::Warning: return plog::warning;
    case LogLevel::Info: return plog::info;
    case LogLevel::Debug: return plog::debug;
    case LogLevel::Verbose: return plog::verbose;
    }
    lunaticvibes::assert_failed("plogSeverityFromLogLevel");
}

std::ostream& operator<<(std::ostream& os, const LogLevel& level)
{
    switch (level)
    {
    case LogLevel::Fatal: return os << "Fatal";
    case LogLevel::Error: return os << "Error";
    case LogLevel::Warning: return os << "Warning";
    case LogLevel::Info: return os << "Info";
    case LogLevel::Debug: return os << "Debug";
    case LogLevel::Verbose: return os << "Verbose";
    }
    lunaticvibes::assert_failed("operator<<(LogLevel)");
}

void SetLogLevel(const LogLevel level)
{
    plog::get()->setMaxSeverity(plogSeverityFromLogLevel(level));
    LOG_INFO << "[Log] log level set to " << level;
}

} // namespace lunaticvibes

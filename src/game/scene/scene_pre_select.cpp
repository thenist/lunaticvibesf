#include "scene_pre_select.h"

#include <cstdlib>
#include <functional>
#include <future>
#include <mutex>
#include <string_view>
#include <utility>

#include <boost/format.hpp>
#include <imgui.h>

#include <common/coursefile/lr2crs.h>
#include <common/entry/entry_arena.h>
#include <common/entry/entry_course.h>
#include <common/entry/entry_table.h>
#include <common/log.h>
#include <common/sysutil.h>
#include <config/config_mgr.h>
#include <db/db_score.h>
#include <db/db_song.h>
#include <game/runtime/i18n.h>
#include <game/runtime/index/number.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <git_version.h>

// TODO: translations.
#define _(String) String

ScenePreSelect::ScenePreSelect() : SceneBase(nullptr, SkinType::PRE_SELECT, 240)
{
    _type = SceneType::PRE_SELECT;

    _state = SceneState::LoadSongs;

    rootFolderProp = SongListProperties{{}, ROOT_FOLDER_HASH, "", {}, {}, 0};

    lunaticvibes::window::graphics_set_maxfps(30);

    LOG_INFO << "[List] ------------------------------------------------------------";

    if (gNextScene == SceneType::PRE_SELECT)
    {
        // score db
        LOG_INFO << "[List] Initializing score.db...";
        g_pScoreDB = std::make_shared<ScoreDB>(ConfigMgr::Profile()->getPath() / "score.db");
        g_pScoreDB->preloadScore();

        // song db
        LOG_INFO << "[List] Initializing song.db...";
        Path dbPath = Path(GAMEDATA_PATH) / "database";
        if (!fs::exists(dbPath))
            fs::create_directories(dbPath);
        g_pSongDB = std::make_shared<SongDB>(dbPath / "song.db");

        std::unique_lock l(gSelectContext._mutex);
        gSelectContext.entries.clear();
        gSelectContext.backtrace.clear();

        {
            std::unique_lock l{_textHintMutex};
            textHint = i18n::s(i18nText::INITIALIZING);
        }
    }

    LOG_INFO << "[List] ------------------------------------------------------------";
}

ScenePreSelect::~ScenePreSelect()
{
    if (loadSongEnd.valid())
        loadSongEnd.get();
    if (loadTableEnd.valid())
        loadTableEnd.get();
    if (loadCourseEnd.valid())
        loadCourseEnd.get();
}

void ScenePreSelect::update_fixed(const lunaticvibes::Time& t)
{
    if (gNextScene != SceneType::PRE_SELECT && gNextScene != SceneType::SELECT)
        return;

    if (gAppIsExiting)
    {
        gNextScene = SceneType::EXIT_TRANS;
        g_pSongDB->stopLoading();
        return;
    }

    switch (_state)
    {
    case SceneState::LoadSongs: updateLoadSongs(); break;
    case SceneState::LoadTables: updateLoadTables(); break;
    case SceneState::LoadCourses: updateLoadCourses(); break;
    case SceneState::UpdateScoreCache: updateUpdateScoreCache(); break;
    case SceneState::Finish: loadFinished(); break;
    }
}

void ScenePreSelect::updateLoadSongs()
{
    if (!startedLoadSong)
    {
        startedLoadSong = true;
        LOG_INFO << "[List] Start loading songs...";

        // TODO: wait for Initializing... text to draw

        // load files
        loadSongEnd = std::async(std::launch::async, [this]() {
            {
                std::unique_lock l{_textHintMutex};
                textHint = i18n::s(i18nText::CHECKING_FOLDERS);
            }

            // get folders from config
            const std::vector<Path> pathList = ConfigMgr::General()->getFoldersPath();

            LOG_INFO << "[List] Refreshing folders...";
            g_pSongDB->initializeFolders(pathList);
            LOG_INFO << "[List] Refreshing folders complete.";

            LOG_INFO << "[List] Building song list cache...";
            g_pSongDB->prepareCache();
            LOG_INFO << "[List] Building song list cache finished.";

            LOG_INFO << "[List] Generating root folders...";
            auto top = g_pSongDB->browse(ROOT_FOLDER_HASH, false);
            if (top && !top->empty())
            {
                for (size_t i = 0; i < top->getContentsCount(); ++i)
                {
                    if (gAppIsExiting)
                        break;
                    auto entry = top->getEntry(i);

                    bool deleted = true;
                    for (const auto& f : pathList)
                    {
                        if (gAppIsExiting)
                            break;
                        if (fs::exists(f) && fs::exists(entry->getPath()) && fs::equivalent(f, entry->getPath()))
                        {
                            deleted = false;
                            break;
                        }
                    }
                    if (!deleted)
                    {
                        g_pSongDB->browse(entry->md5, true);
                        rootFolderProp.dbBrowseEntries.emplace_back(std::move(entry), nullptr);
                    }
                }
            }
            if (gAppIsExiting)
                return;
            LOG_INFO << "[List] Added " << rootFolderProp.dbBrowseEntries.size() << " root folders";

            g_pSongDB->optimize();

            // NEW SONG
            LOG_INFO << "[List] Generating NEW SONG folder...";

            if (auto newSongList = g_pSongDB->findChartFromTime(
                    ROOT_FOLDER_HASH, getFileTimeNow() - State::get(IndexNumber::NEW_ENTRY_SECONDS));
                !newSongList.empty())
            {
                LOG_INFO << "[List] Adding " << newSongList.size() << " entries to NEW SONGS";
                auto entry = std::make_shared<EntryFolderNewSong>("NEW SONGS");
                for (auto&& c : newSongList)
                {
                    if (gAppIsExiting)
                        break;
                    entry->pushEntry(std::make_shared<EntryFolderSong>(std::move(c)));
                }
                rootFolderProp.dbBrowseEntries.insert(rootFolderProp.dbBrowseEntries.begin(), {entry, nullptr});
            }
            else
            {
                LOG_INFO << "[List] No NEW SONG entries";
            }

            // ARENA
            LOG_INFO << "[List] Generating ARENA folder...";
            if (!rootFolderProp.dbBrowseEntries.empty())
            {
                auto entry = std::make_shared<EntryFolderArena>(i18n::s(i18nText::ARENA_FOLDER_TITLE),
                                                                i18n::s(i18nText::ARENA_FOLDER_SUBTITLE));

                entry->pushEntry(std::make_shared<EntryArenaCommand>(EntryArenaCommand::Type::HOST_LOBBY,
                                                                     i18n::s(i18nText::ARENA_HOST),
                                                                     i18n::s(i18nText::ARENA_HOST_DESCRIPTION)));
                entry->pushEntry(std::make_shared<EntryArenaCommand>(EntryArenaCommand::Type::JOIN_LOBBY,
                                                                     i18n::s(i18nText::ARENA_JOIN),
                                                                     i18n::s(i18nText::ARENA_JOIN_DESCRIPTION)));
                entry->pushEntry(std::make_shared<EntryArenaCommand>(EntryArenaCommand::Type::LEAVE_LOBBY,
                                                                     i18n::s(i18nText::ARENA_LEAVE),
                                                                     i18n::s(i18nText::ARENA_LEAVE_DESCRIPTION)));

                // TODO load lobby list from file

                rootFolderProp.dbBrowseEntries.emplace_back(std::move(entry), nullptr);
            }
            LOG_INFO << "[List] ARENA has " << 0 << " known hosts (placeholder)";
        });
    }

    if (g_pSongDB->addChartTaskFinishCount != prevChartLoaded)
    {
        std::shared_lock l{g_pSongDB->addCurrentPathMutex, std::defer_lock};
        std::unique_lock l2{_textHintMutex, std::defer_lock};
        std::lock(l, l2);

        prevChartLoaded = g_pSongDB->addChartTaskFinishCount;
        textHint = (boost::format(i18n::c(i18nText::LOADING_CHARTS)) % g_pSongDB->addChartTaskFinishCount %
                    g_pSongDB->addChartTaskCount)
                       .str();
        textHint2 = g_pSongDB->addCurrentPath;
    }

    if (loadSongEnd.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        g_pSongDB->waitLoadingFinish();
        loadSongEnd.get();
        LOG_INFO << "[List] Loading songs complete.";
        LOG_INFO << "[List] ------------------------------------------------------------";
        _state = SceneState::LoadTables;
    }
}

static const std::string_view to_str(const DifficultyTable::UpdateResult result)
{
    switch (result)
    {
    case DifficultyTable::UpdateResult::OK: return "OK";
    case DifficultyTable::UpdateResult::INTERNAL_ERROR: return "INTERNAL_ERROR";
    case DifficultyTable::UpdateResult::WEB_PATH_ERROR: return "WEB_PATH_ERROR";
    case DifficultyTable::UpdateResult::WEB_CONNECT_ERR: return "WEB_CONNECT_ERR";
    case DifficultyTable::UpdateResult::WEB_TIMEOUT: return "WEB_TIMEOUT";
    case DifficultyTable::UpdateResult::WEB_PARSE_FAILED: return "WEB_PARSE_FAILED";
    case DifficultyTable::UpdateResult::HEADER_PATH_ERROR: return "HEADER_PATH_ERROR";
    case DifficultyTable::UpdateResult::HEADER_CONNECT_ERR: return "HEADER_CONNECT_ERR";
    case DifficultyTable::UpdateResult::HEADER_TIMEOUT: return "HEADER_TIMEOUT";
    case DifficultyTable::UpdateResult::HEADER_PARSE_FAILED: return "HEADER_PARSE_FAILED";
    case DifficultyTable::UpdateResult::DATA_PATH_ERROR: return "DATA_PATH_ERROR";
    case DifficultyTable::UpdateResult::DATA_CONNECT_ERR: return "DATA_CONNECT_ERR";
    case DifficultyTable::UpdateResult::DATA_TIMEOUT: return "DATA_TIMEOUT";
    case DifficultyTable::UpdateResult::DATA_PARSE_FAILED: return "DATA_PARSE_FAILED";
    }
    lunaticvibes::assert_failed("to_str(DifficultyTable::UpdateResult)");
}

void ScenePreSelect::updateLoadTables()
{
    if (!startedLoadTable)
    {
        startedLoadTable = true;
        LOG_INFO << "[List] Start loading tables...";

        loadTableEnd = std::async(std::launch::async, [this]() {
            {
                std::unique_lock l{_textHintMutex};
                textHint = i18n::s(i18nText::CHECKING_TABLES);
            }

            // initialize table list
            auto tableList = ConfigMgr::General()->getTablesUrl();
            size_t tableIndex = 0;
            for (auto& tableUrl : tableList)
            {
                if (gAppIsExiting)
                    break;
                LOG_INFO << "[List] Add table " << tableUrl;
                {
                    std::unique_lock l{_textHintMutex};
                    textHint2 = tableUrl;
                }

                gSelectContext.tables.emplace_back();
                DifficultyTableBMS& t = gSelectContext.tables.back();
                t.setUrl(tableUrl);

                auto convertTable = [&](DifficultyTableBMS& t) {
                    auto tbl = std::make_shared<EntryFolderTable>(t.getName(), tableIndex);
                    size_t levelIndex = 0;
                    for (const auto& lv : t.getLevelList())
                    {
                        auto tblLevel = std::make_shared<EntryFolderTable>(t.getSymbol() + lv, levelIndex);
                        for (const auto& r : t.getEntryList(lv))
                        {
                            if (gAppIsExiting)
                                break;
                            auto charts = g_pSongDB->findChartByHash(r->md5, false);
                            for (auto& c : charts)
                            {
                                if (fs::exists(c->absolutePath))
                                {
                                    tblLevel->pushEntry(std::make_shared<EntryFolderSong>(c));
                                    break;
                                }
                            }
                        }
                        tbl->pushEntry(std::move(tblLevel));
                        levelIndex += 1;
                    }
                    return tbl;
                };

                {
                    std::unique_lock l{_textHintMutex};
                    textHint = (boost::format(i18n::c(i18nText::LOADING_TABLE)) % t.getUrl()).str();
                    textHint2 = "";
                }

                if (t.loadFromFile())
                {
                    // TODO should re-download the table if outdated
                    LOG_INFO << "[List] Local table file found: " << t.getFolderPath();
                    rootFolderProp.dbBrowseEntries.emplace_back(convertTable(t), nullptr);
                }
                else
                {
                    LOG_INFO << "[List] Local file not found. Downloading... " << t.getFolderPath();
                    {
                        std::unique_lock l{_textHintMutex};
                        textHint2 = "";
                    }

                    t.updateFromUrl([&](DifficultyTable::UpdateResult result) {
                        if (result == DifficultyTable::UpdateResult::OK)
                        {
                            LOG_INFO << "[List] Table file download complete: " << t.getFolderPath();
                            rootFolderProp.dbBrowseEntries.emplace_back(convertTable(t), nullptr);
                        }
                        else
                        {
                            LOG_WARNING << "[List] Update table " << tableUrl << " failed: " << to_str(result);
                        }
                    });
                    tableIndex += 1;
                }
            }
        });
    }

    if (loadTableEnd.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        loadTableEnd.get();
        LOG_INFO << "[List] Loading tables complete.";
        LOG_INFO << "[List] ------------------------------------------------------------";
        _state = SceneState::LoadCourses;
    }
}

void ScenePreSelect::updateLoadCourses()
{
    if (!startedLoadCourse)
    {
        startedLoadCourse = true;
        LOG_INFO << "[List] Start loading courses...";

        loadCourseEnd = std::async(std::launch::async, [this]() {
            {
                std::unique_lock l{_textHintMutex};
                textHint = i18n::s(i18nText::LOADING_COURSES);
            }

            std::map<EntryCourse::CourseType, std::vector<std::shared_ptr<EntryCourse>>> courses;

            // initialize table list
            Path coursePath = Path(GAMEDATA_PATH) / "courses";
            if (!fs::exists(coursePath))
                fs::create_directories(coursePath);

            LOG_INFO << "[List] Loading courses from courses/*.lr2crs...";
            for (auto& courseFile : fs::recursive_directory_iterator(coursePath))
            {
                if (!(fs::is_regular_file(courseFile) &&
                      lunaticvibes::iequals(lunaticvibes::s(courseFile.path().extension().u8string()), ".lr2crs")))
                    continue;

                const Path& coursePath = courseFile.path();
                LOG_INFO << "[List] Loading course file: " << coursePath;

                {
                    std::unique_lock l{_textHintMutex};
                    textHint2 = lunaticvibes::u8str(coursePath);
                }

                CourseLr2crs lr2crs(coursePath);
                for (auto& c : lr2crs.courses)
                {
                    auto entry = std::make_shared<EntryCourse>(c, lr2crs.addTime);
                    if (entry->courseType != EntryCourse::UNDEFINED)
                        courses[entry->courseType].push_back(std::move(entry));
                }
            }
            LOG_INFO << "[List] *.lr2crs loading complete.";

            // TODO load courses from tables

            for (auto& [type, courses] : courses)
            {
                if (courses.empty())
                    continue;

                std::string folderTitle;
                std::string folderTitle2;
                switch (type)
                {
                case EntryCourse::CourseType::UNDEFINED:
                    folderTitle = i18n::s(i18nText::COURSE_TITLE);
                    folderTitle2 = i18n::s(i18nText::COURSE_SUBTITLE);
                    break;
                case EntryCourse::CourseType::GRADE:
                    folderTitle = i18n::s(i18nText::CLASS_TITLE);
                    folderTitle2 = i18n::s(i18nText::CLASS_SUBTITLE);
                    break;
                }

                auto folder = std::make_shared<EntryFolderCourse>(folderTitle, folderTitle2);
                for (auto& c : courses)
                {
                    LOG_INFO << "[List] Add course: " << c->_name;
                    folder->pushEntry(c);
                }
                rootFolderProp.dbBrowseEntries.emplace_back(std::move(folder), nullptr);
            }
        });
    }

    if (loadCourseEnd.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        loadCourseEnd.get();
        LOG_INFO << "[List] Loading courses complete.";
        LOG_INFO << "[List] ------------------------------------------------------------";
        _state = SceneState::UpdateScoreCache;
    }
}

void ScenePreSelect::updateUpdateScoreCache()
{
    if (!startedUpdateScoreCache)
    {
        startedUpdateScoreCache = true;
        LOG_INFO << "[List] Start updating score cache...";

        updateScoreCacheEnd = std::async(std::launch::async, [this]() {
            {
                std::unique_lock l{_textHintMutex};
                textHint = _("Updating score cache...");
                textHint2.clear();
            }
            if (g_pScoreDB->isBmsPbCacheEmpty())
                g_pScoreDB->rebuildBmsPbCache();
        });
    }

    if (updateScoreCacheEnd.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        updateScoreCacheEnd.get();
        LOG_INFO << "[List] Finished updating score cache";
        LOG_INFO << "[List] ------------------------------------------------------------";
        _state = SceneState::Finish;
    }
}

void ScenePreSelect::loadFinished()
{
    // Allow a single frame with hint to get rendered as we may get stuck with this frame for a view seconds while the
    // skin is loading.
    bool ready = true;

    if (!_preparedForFinish)
    {
        _preparedForFinish = true;
        ready = false;

        gSelectContext.backtrace.clear();
        gSelectContext.backtrace.push_front(rootFolderProp);

        if (rootFolderProp.dbBrowseEntries.empty())
        {
            State::set(IndexText::PLAY_TITLE, i18n::s(i18nText::BMS_NOT_FOUND));
            State::set(IndexText::PLAY_ARTIST, i18n::s(i18nText::BMS_NOT_FOUND_HINT));
        }
        if (gNextScene == SceneType::PRE_SELECT)
        {
            std::unique_lock l{_textHintMutex};
            textHint = std::format("{} {} {} ({})", PROJECT_NAME, PROJECT_VERSION,
#ifndef NDEBUG
                                   "Debug",
#else
                                   "",
#endif
                                   GIT_REVISION);
            textHint2 = i18n::s(i18nText::PLEASE_WAIT);
        }
    }

    if (ready && !_switchedScene)
    {
        int maxFPS = ConfigMgr::General()->get(cfg::V_MAXFPS, 480);
        if (maxFPS < 30 && maxFPS != 0)
            maxFPS = 30;
        lunaticvibes::window::graphics_set_maxfps(maxFPS);

        gNextScene = SceneType::SELECT;
        _switchedScene = true;
    }
}

bool ScenePreSelect::shouldShowImgui() const
{
    return true;
}

void ScenePreSelect::updateImgui()
{
    if (gInCustomize)
        return;

    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(ConfigMgr::General()->get(cfg::V_DISPLAY_RES_X, CANVAS_WIDTH)),
                                    static_cast<float>(ConfigMgr::General()->get(cfg::V_DISPLAY_RES_Y, CANVAS_HEIGHT))),
                             ImGuiCond_Always);
    if (ImGui::Begin("LoadSong", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoCollapse))
    {
        std::unique_lock l{_textHintMutex};
        ImGui::TextUnformatted(textHint.c_str());
        ImGui::TextUnformatted(textHint2.c_str());

        ImGui::End();
    }
}

bool ScenePreSelect::isLoadingFinished() const
{
    return _switchedScene;
}

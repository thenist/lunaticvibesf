#pragma once
#include "scene.h"
#include "scene_context.h"

#include <common/types.h>

class ScenePreSelect final : public SceneBase
{
public:
    ScenePreSelect();
    ~ScenePreSelect() override;
    void update_fixed(const lunaticvibes::Time& t) override;

protected:
    enum class SceneState : uint8_t
    {
        LoadSongs,
        LoadTables,
        LoadCourses,
        UpdateScoreCache,
        Finish,
    };
    SceneState _state;

    void updateLoadSongs();
    void updateLoadTables();
    void updateLoadCourses();
    void updateUpdateScoreCache();
    void loadFinished();

    [[nodiscard]] bool shouldShowImgui() const override;
    void updateImgui() override;

protected:
    SongListProperties rootFolderProp;
    bool startedLoadSong = false;
    bool startedLoadTable = false;
    bool startedLoadCourse = false;
    bool startedUpdateScoreCache = false;
    std::future<void> loadSongEnd;
    std::future<void> loadTableEnd;
    std::future<void> loadCourseEnd;
    std::future<void> updateScoreCacheEnd;
    int prevChartLoaded = 0;
    std::string textHint;
    std::string textHint2;

    bool _preparedForFinish = false;
    bool _switchedScene = false;

public:
    [[nodiscard]] bool isLoadingFinished() const;
};

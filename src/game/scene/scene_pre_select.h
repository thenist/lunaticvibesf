#pragma once
#include "scene.h"
#include "scene_context.h"

class ScenePreSelect : public SceneBase
{
public:
    ScenePreSelect();
    ~ScenePreSelect() override;

protected:
    // Looper callbacks
    enum class SceneState
    {
        LoadSongs,
        LoadTables,
        LoadCourses,
        UpdateScoreCache,
        Finish,
    };
    SceneState _state;
    bool readyToStopAsync() const override;
    void _updateAsync() override;

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

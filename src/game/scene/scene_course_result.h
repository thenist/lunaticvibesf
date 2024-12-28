#pragma once

#include "scene.h"

#include <game/skin/skin_mgr.h>

#include <memory>

class ScoreBase;

class SceneCourseResult final : public SceneBase
{
public:
    explicit SceneCourseResult(const std::shared_ptr<SkinMgr>& skinMgr);
    ~SceneCourseResult() override;

private:
    enum class eCourseResultState
    {
        DRAW,
        STOP,
        RECORD,
        FADEOUT,
    };
    eCourseResultState state;
    InputMask _inputAvailable;

protected:
    bool _scoreSyncFinished = false;
    bool _retryRequested = false;
    std::shared_ptr<ScoreBase> _pScoreOld;

    enum SummaryArgs
    {
        ARG_TOTAL_NOTES,
        ARG_MAX_SCORE,
        ARG_SCORE,
        ARG_EXSCORE,
        ARG_MAXCOMBO,
        ARG_FAST,
        ARG_SLOW,
        ARG_BP,
        ARG_CB,

        ARG_JUDGE_0,
        ARG_JUDGE_1,
        ARG_JUDGE_2,
        ARG_JUDGE_3,
        ARG_JUDGE_4,
        ARG_JUDGE_5,
    };
    std::map<SummaryArgs, unsigned> summary;
    double acc = 0.;

protected:
    void update_fixed(const lunaticvibes::Time& t) override;
    void updateDraw(const lunaticvibes::Time& t);
    void updateStop(const lunaticvibes::Time& t);
    void updateRecord(const lunaticvibes::Time& t);
    void updateFadeout(const lunaticvibes::Time& t);

protected:
    // Register to InputWrapper: judge / keysound
    void inputGamePress(InputMask&, const lunaticvibes::Time&);
    void inputGameHold(InputMask&, const lunaticvibes::Time&);
    void inputGameRelease(InputMask&, const lunaticvibes::Time&);
};

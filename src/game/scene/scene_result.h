#pragma once
#include "scene.h"
#include <memory>

class ScoreBase;
class SkinMgr;

class SceneResult final : public SceneBase
{
public:
    explicit SceneResult(const std::shared_ptr<SkinMgr>& skinMgr);
    ~SceneResult() override;

protected:
    enum class eResultState
    {
        DRAW,
        STOP,
        RECORD,
        FADEOUT,
        WAIT_ARENA,
    };

    void update_fixed(const lunaticvibes::Time& t) override;
    void updateDraw(const lunaticvibes::Time& t);
    void updateStop(const lunaticvibes::Time& t);
    void updateRecord(const lunaticvibes::Time& t);
    void updateFadeout(const lunaticvibes::Time& t);
    void updateWaitArena(const lunaticvibes::Time& t);

protected:
    // Register to InputWrapper: judge / keysound
    void inputGamePress(InputMask&, const lunaticvibes::Time&);
    void inputGameHold(InputMask&, const lunaticvibes::Time&);

private:
    eResultState state;
    InputMask _inputAvailable;

protected:
    bool _scoreSyncFinished = false;
    bool _retryRequested = false;
    bool _savedScore = false;
    std::shared_ptr<ScoreBase> _pScoreOld;

    bool saveScore = false;
    ScoreBMS::Lamp saveLampMax;
    ScoreBMS::Lamp lamp[2];
};

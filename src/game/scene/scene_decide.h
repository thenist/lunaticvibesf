#pragma once

#include <game/scene/scene.h>

class SceneDecide : public SceneBase
{
public:
    explicit SceneDecide(const std::shared_ptr<SkinMgr>& skinMgr);
    ~SceneDecide() override;

protected:
    // Looper callbacks
    enum class eDecideState
    {
        START,
        SKIP,
        CANCEL,
    };
    void _updateAsync() override;
    void updateStart();
    void updateSkip();
    void updateCancel();

protected:
    // Register to InputWrapper: judge / keysound
    void inputGamePress(InputMask&, const lunaticvibes::Time&);
    void inputGameHold(InputMask&, const lunaticvibes::Time&);
    void inputGameRelease(InputMask&, const lunaticvibes::Time&);

private:
    eDecideState state;
    InputMask _inputAvailable;
};

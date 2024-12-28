#pragma once

#include <game/scene/scene.h>

class SceneDecide final : public SceneBase
{
public:
    explicit SceneDecide(const std::shared_ptr<SkinMgr>& skinMgr);
    ~SceneDecide() override;

protected:
    enum class eDecideState
    {
        START,
        SKIP,
        CANCEL,
    };
    void update_fixed(const lunaticvibes::Time& t) override;
    void updateStart(const lunaticvibes::Time& t);
    void updateSkip(const lunaticvibes::Time& t);
    void updateCancel(const lunaticvibes::Time& t);

protected:
    // Register to InputWrapper: judge / keysound
    void inputGamePress(InputMask&, const lunaticvibes::Time&);
    void inputGameHold(InputMask&, const lunaticvibes::Time&);
    void inputGameRelease(InputMask&, const lunaticvibes::Time&);

private:
    eDecideState state;
    InputMask _inputAvailable;
};

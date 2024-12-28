#pragma once
#include "game/skin/skin_mgr.h"
#include "scene.h"
#include <memory>

namespace lunaticvibes
{

enum class CustomizeState
{
    Start,
    Main,
    Fadeout,
};

} // namespace lunaticvibes

class SceneCustomize : public SceneBase
{
public:
    explicit SceneCustomize(const std::shared_ptr<SkinMgr>& skinMgr);
    ~SceneCustomize() override;

    void setIsVirtual(bool is_virtual) { _is_virtual = is_virtual; };

protected:
    struct Option
    {
        int id;
        StringContent displayName;
        std::vector<StringContent> entries;
        size_t selectedEntry;
    };
    size_t topOptionIndex = 0;
    std::map<StringContent, Option> optionsMap;
    std::vector<StringContent> optionsKeyList;

    std::map<SkinType, std::vector<Path>> skinList;
    std::vector<Path> soundsetList;

    SkinType selectedMode;

protected:
    // Looper callbacks
    void _updateAsync() override {};

    void update() override;
    void updateStart();
    void updateMain();
    void updateFadeout();

public:
    static StringPath getConfigFileName(StringPathView skinPath);

protected:
    void setOption(size_t idxOption, size_t idxEntry);
    void load(SkinType mode);
    void save(SkinType mode) const;
    void updateTexts() const;

protected:
    void inputGamePress(InputMask&, const lunaticvibes::Time&);

public:
    void draw() const override;

private:
    std::shared_ptr<SkinMgr> _skinMgr;
    std::shared_ptr<Texture> graphics_get_screen_texture;
    lunaticvibes::CustomizeState _state;
    bool exiting = false;
    bool _is_virtual = false;
};

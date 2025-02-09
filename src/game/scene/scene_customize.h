#pragma once

#include "scene.h"

#include <common/beat.h>
#include <common/types.h>
#include <game/skin/skin_mgr.h>

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <vector>

using std::size_t;

namespace lunaticvibes
{

enum class CustomizeState : uint8_t
{
    Start,
    Main,
    Fadeout,
};

} // namespace lunaticvibes

class SceneCustomize final : public SceneBase
{
public:
    SceneCustomize(std::shared_ptr<SkinMgr> skinMgr, std::optional<SkinType> mode);
    ~SceneCustomize() override;

    void update_fixed(const lunaticvibes::Time& t) override;

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

    bool _handleInput = false;

protected:
    void updateStart(const lunaticvibes::Time& t);
    void updateMain(const lunaticvibes::Time& t);
    void updateFadeout(const lunaticvibes::Time& t);

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

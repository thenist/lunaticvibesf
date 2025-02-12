#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>

#include <common/assert.h>
#include <config/cfg_general.h>
#include <config/cfg_input.h>
#include <config/cfg_profile.h>
#include <config/cfg_skin.h>

class ConfigMgr
{
private:
    ConfigMgr() = default;
    ~ConfigMgr() = default;
    static ConfigMgr& getInst()
    {
        static ConfigMgr inst;
        return inst;
    }

    void _init();
    void _load();
    void _save();
    int _selectProfile(const std::string& name);
    int _createProfile(const std::string& newProfile, const std::string& oldProfile);
    void _setGlobals();

    template <class Ty_v> Ty_v _get(char type, const std::string& key, const Ty_v& fallback)
    {
        std::shared_lock l(_mutex);
        switch (type)
        {
        case 'A':
        case 'E':
        case 'V': return G->get<Ty_v>(key, fallback);
        case 'S': return S->get<Ty_v>(key, fallback);
        }
        lunaticvibes::assert_failed("ConfigMgr::_get");
    }
    template <class Ty_v> void _set(char type, const std::string& key, const Ty_v& value) noexcept
    {
        std::unique_lock l(_mutex);
        switch (type)
        {
        case 'A':
        case 'E':
        case 'V': return G->set<Ty_v>(key, value);
        case 'S': return S->set<Ty_v>(key, value);
        }
        lunaticvibes::assert_failed("ConfigMgr::_set");
    }

protected:
    std::shared_ptr<ConfigGeneral> G;
    std::shared_ptr<ConfigProfile> P;
    std::shared_ptr<ConfigInput> I5;
    std::shared_ptr<ConfigInput> I7;
    std::shared_ptr<ConfigInput> I9;
    std::shared_ptr<ConfigSkin> S;
    std::string profileName;
    std::shared_mutex _mutex;

public:
    std::shared_ptr<ConfigGeneral> General1() { return G; }
    std::shared_ptr<ConfigProfile> Profile1() { return P; }
    std::shared_ptr<ConfigSkin> Skin1() { return S; };
    std::shared_ptr<ConfigInput> Input1(GameModeKeys mode)
    {
        switch (mode)
        {
        case 5: return I5;
        case 7: return I7;
        case 9: return I9;
        default: lunaticvibes::assert_failed("ConfigMgr::Input1");
        }
    }

public:
    static void init() { getInst()._init(); }
    static void load() { getInst()._load(); }
    static void save() { getInst()._save(); }
    static int selectProfile(const std::string& name) { return getInst()._selectProfile(name); }
    static int createProfile(const std::string& newProfile, const std::string& oldProfile)
    {
        return getInst()._createProfile(newProfile, oldProfile);
    }
    static void setGlobals() { getInst()._setGlobals(); }
    static std::shared_ptr<ConfigGeneral> General() { return getInst().General1(); }
    static std::shared_ptr<ConfigProfile> Profile() { return getInst().Profile1(); }
    static std::shared_ptr<ConfigInput> Input(GameModeKeys mode) { return getInst().Input1(mode); }
    static std::shared_ptr<ConfigSkin> Skin() { return getInst().Skin1(); }

    template <class Ty_v> static Ty_v get(char type, const std::string& key, const Ty_v& fallback)
    {
        static_assert(!std::is_same_v<Ty_v, std::string_view>, "string_view isn't supported by YAML-cpp");
        return getInst()._get(type, key, fallback);
    }

    static std::string get(char type, const std::string& key, const std::string& fallback)
    {
        return get<std::string>(type, key, fallback);
    }

    template <class Ty_v> static void set(char type, const std::string& key, const Ty_v& value) noexcept
    {
        static_assert(!std::is_same_v<Ty_v, std::string_view>, "string_view isn't supported by YAML-cpp");
        return getInst()._set(type, key, value);
    }

    static void set(char type, const std::string& key, const std::string& value) noexcept
    {
        return set<std::string>(type, key, value);
    }

public:
};

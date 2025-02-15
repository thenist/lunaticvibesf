#pragma once

#include <memory>
#include <shared_mutex>
#include <string>

#include <common/assert.h>
#include <common/types.h>

class ConfigGeneral;
class ConfigSkin;
class ConfigInput;
class ConfigProfile;

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
};

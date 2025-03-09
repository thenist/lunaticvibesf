#pragma once

#include <common/beat.h>
#include <common/types.h>
#include <game/sound/sound_driver.h>

#include <fmod.hpp>

#include <array>
#include <atomic>
#include <map>
#include <string>

// This game uses FMOD Low Level API to play sounds as we don't use FMOD Studio,

class SoundDriverFMOD final : public SoundDriver
{
    friend class SoundMgr;

private:
    FMOD::System* fmodSystem = nullptr;
    int initRet;

protected:
    std::map<SoundChannelType, std::shared_ptr<FMOD::ChannelGroup>> channelGroup;
    std::map<SampleChannel, float> volume;
    std::atomic<float> sysVolume = 1.0;
    std::atomic<float> noteVolume = 1.0;

    std::atomic<float> sysVolumeGradientBegin = 1.0;
    std::atomic<float> sysVolumeGradientEnd = 1.0;
    std::atomic<long long> sysVolumeGradientBeginTime;
    std::atomic<int> sysVolumeGradientLength = 0;
    std::atomic<float> noteVolumeGradientBegin = 1.0;
    std::atomic<float> noteVolumeGradientEnd = 1.0;
    std::atomic<long long> noteVolumeGradientBeginTime;
    std::atomic<int> noteVolumeGradientLength = 0;

    std::map<SoundChannelType, FMOD::DSP*> DSPMaster[3];
    std::map<SoundChannelType, FMOD::DSP*> DSPKey[3];
    std::map<SoundChannelType, FMOD::DSP*> DSPBgm[3];
    std::map<SoundChannelType, FMOD::DSP*> PitchShiftFilter;
    std::map<SoundChannelType, FMOD::DSP*> EQFilter[2];

public:
    static constexpr size_t BMS_NUMBER_BASE = 62;
    static constexpr size_t NOTESAMPLES = BMS_NUMBER_BASE * BMS_NUMBER_BASE + 1;
    static constexpr size_t SYSSAMPLES = 64;
    struct SoundSample
    {
        FMOD::Sound* objptr = nullptr;
        std::string path;
        int flags = 0;
    };

protected:
    std::array<SoundSample, NOTESAMPLES> noteSamples{}; // Sound samples of key sound
    std::array<SoundSample, SYSSAMPLES> sysSamples{};   // Sound samples of BGM, effect, etc

public:
    SoundDriverFMOD();
    ~SoundDriverFMOD() override;
    void createChannelGroups();

public:
    std::vector<std::pair<int, std::string>> getDeviceList() override;
    int setDevice(size_t index) override;
    std::pair<int, int> getDSPBufferSize() override;

private:
    int findDriver(const std::string& name, int driverIDUnknown);

public:
    int loadNoteSample(const Path& path, size_t index) override;
    void playNoteSample(SoundChannelType ch, size_t count, size_t index[]) override;
    void stopNoteSamples() override;
    void freeNoteSamples() override;
    long long getNoteSampleLength(size_t index) override;
    virtual void update();

public:
    int loadSysSample(const Path& path, size_t index, bool isStream = false, bool loop = false) override;
    void playSysSample(SoundChannelType ch, size_t index) override;
    void stopSysSamples() override;
    void freeSysSamples() override;
    int getChannelsPlaying();

public:
    void setSysVolume(float v, int gradientTime = 0) override;
    void setNoteVolume(float v, int gradientTime = 0) override;
    void setVolume(SampleChannel ch, float v) override;
    void setDSP(DSPType type, int index, SampleChannel ch, float p1, float p2) override;
    void setFreqFactor(double f) override;
    void setSpeed(double speed) override;
    void setPitch(double pitch) override;
    void setEQ(EQFreq freq, int gain) override;
};

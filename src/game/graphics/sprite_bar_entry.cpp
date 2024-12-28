#include "sprite_bar_entry.h"

#include <shared_mutex>

#include "common/chartformat/chartformat_bms.h"
#include "common/entry/entry_types.h"
#include "common/log.h"
#include "game/scene/scene_context.h"
#include <common/assert.h>

int SpriteBarEntry::setBody(BarType type, const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarType::TYPE_COUNT))
    {
        LOG_DEBUG << "[Sprite] BarBody type (" << int(type) << "Invalid!"
                  << " (Line " << srcLine << ")";
        return 1;
    }

    auto idx = static_cast<size_t>(type);

    sBodyOff[idx] = builder.build();

    sBodyOn[idx] = builder.build();

    return 0;
}

int SpriteBarEntry::setFlash(const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    sFlash = builder.build();
    return 0;
}

int SpriteBarEntry::setLevel(BarLevelType type, const SpriteNumber::SpriteNumberBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarLevelType::LEVEL_TYPE_COUNT))
    {
        LOG_DEBUG << "[Sprite] BarEntry level type (" << int(type) << "Invalid!"
                  << " (Line " << srcLine << ")";
        return 1;
    }

    if (builder.maxDigits < 3 && type == BarLevelType::IRRANK)
    {
        LOG_DEBUG << "[Sprite] BarEntry level digit (" << builder.maxDigits << ") not enough for IRRANK "
                  << " (Line " << srcLine << ")";
    }
    else if (builder.maxDigits < 2)
    {
        LOG_DEBUG << "[Sprite] BarEntry level digit (" << builder.maxDigits << ") not enough for idx " << int(type)
                  << " (Line " << srcLine << ")";
    }

    SpriteNumber::SpriteNumberBuilder tmpBuilder = builder;
    tmpBuilder.numInd = IndexNumber(unsigned(IndexNumber::_SELECT_BAR_LEVEL_0) + index);
    sLevel[static_cast<size_t>(type)] = tmpBuilder.build();
    return 0;
}

int SpriteBarEntry::setLamp(BarLampType type, const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarLampType::LAMP_TYPE_COUNT))
    {
        LOG_DEBUG << "[Sprite] BarEntry lamp type (" << int(type) << "Invalid!"
                  << " (Line " << srcLine << ")";
        return 1;
    }

    sLamp[static_cast<size_t>(type)] = builder.build();
    return 0;
}

int SpriteBarEntry::setTitle(BarTitleType type, const SpriteText::SpriteTextBuilder& builder)
{
    SpriteText::SpriteTextBuilder tmpBuilder = builder;
    tmpBuilder.textInd = IndexText(int(IndexText::_SELECT_BAR_TITLE_FULL_0) + index);
    sTitle[static_cast<size_t>(type)] = tmpBuilder.build();
    return 0;
}

int SpriteBarEntry::setTitle(BarTitleType type, const SpriteImageText::SpriteImageTextBuilder& builder)
{
    SpriteImageText::SpriteImageTextBuilder tmpBuilder = builder;
    tmpBuilder.textInd = IndexText(int(IndexText::_SELECT_BAR_TITLE_FULL_0) + index);
    sTitle[static_cast<size_t>(type)] = tmpBuilder.build();
    return 0;
}

int SpriteBarEntry::setRank(BarRankType type, const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarRankType::RANK_TYPE_COUNT))
    {
        if (srcLine >= 0)
        {
            LOG_DEBUG << "[Sprite] BarEntry rank type (" << int(type) << "Invalid!"
                      << " (Line " << srcLine << ")";
        }
        return 1;
    }

    sRank[static_cast<size_t>(type)] = builder.build();
    return 0;
}

int SpriteBarEntry::setRivalWinLose(BarRivalType type, const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarRivalType::RIVAL_TYPE_COUNT))
    {
        if (srcLine >= 0)
        {
            LOG_DEBUG << "[Sprite] BarEntry rival type (" << int(type) << ") Invalid!"
                      << " (Line " << srcLine << ")";
        }
        return 1;
    }

    sRivalWinLose[static_cast<size_t>(type)] = builder.build();
    return 0;
}

int SpriteBarEntry::setRivalLampSelf(BarLampType type, const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarLampType::LAMP_TYPE_COUNT))
    {
        if (srcLine >= 0)
        {
            LOG_DEBUG << "[Sprite] BarEntry rival lamp self type (" << int(type) << "Invalid!"
                      << " (Line " << srcLine << ")";
        }
        return 1;
    }

    sRivalLampSelf[static_cast<size_t>(type)] = builder.build();
    return 0;
}

int SpriteBarEntry::setRivalLampRival(BarLampType type, const SpriteAnimated::SpriteAnimatedBuilder& builder)
{
    if (static_cast<size_t>(type) >= static_cast<size_t>(BarLampType::LAMP_TYPE_COUNT))
    {
        if (srcLine >= 0)
        {
            LOG_DEBUG << "[Sprite] BarEntry rival lamp rival type (" << int(type) << "Invalid!"
                      << " (Line " << srcLine << ")";
        }
        return 1;
    }

    sRivalLampRival[static_cast<size_t>(type)] = builder.build();
    return 0;
}

static BarType entry_bar_type(eEntryType e)
{
    switch (e)
    {
    case eEntryType::UNKNOWN: return BarType::SONG;
    case eEntryType::FOLDER: return BarType::FOLDER;
    case eEntryType::NEW_SONG_FOLDER: return BarType::NEW_SONG_FOLDER;
    case eEntryType::CUSTOM_FOLDER: return BarType::CUSTOM_FOLDER;
    case eEntryType::COURSE_FOLDER: return BarType::COURSE_FOLDER;
    case eEntryType::SONG:
    case eEntryType::CHART: return BarType::SONG;
    case eEntryType::RIVAL: return BarType::RIVAL;
    case eEntryType::RIVAL_SONG:
    case eEntryType::RIVAL_CHART: return BarType::SONG_RIVAL;
    case eEntryType::NEW_COURSE: return BarType::NEW_COURSE;
    case eEntryType::COURSE: return BarType::COURSE;
    case eEntryType::RANDOM_COURSE: return BarType::RANDOM_COURSE;
    case eEntryType::ARENA_FOLDER:
    case eEntryType::ARENA_COMMAND: return BarType::RIVAL;
    case eEntryType::ARENA_LOBBY: return BarType::SONG;
    case eEntryType::RANDOM_CHART: return BarType::CUSTOM_FOLDER;
    case eEntryType::CHART_LINK:
    case eEntryType::REPLAY: break;
    }
    return BarType::SONG;
}

static BarLampType score_to_bar_lamp_lr2(ScoreBMS::Lamp lamp)
{
    switch (lamp)
    {
    case ScoreBMS::Lamp::NOPLAY: return BarLampType::NOPLAY;
    case ScoreBMS::Lamp::FAILED:
    case ScoreBMS::Lamp::ASSIST: return BarLampType::FAILED;
    case ScoreBMS::Lamp::EASY: return BarLampType::EASY;
    case ScoreBMS::Lamp::NORMAL: return BarLampType::NORMAL;
    case ScoreBMS::Lamp::HARD:
    case ScoreBMS::Lamp::EXHARD: return BarLampType::HARD;
    case ScoreBMS::Lamp::FULLCOMBO:
    case ScoreBMS::Lamp::PERFECT:
    case ScoreBMS::Lamp::MAX: return BarLampType::FULLCOMBO;
    }
    lunaticvibes::assert_failed("score_to_bar_lamp_lr2");
};

static BarLampType score_to_bar_lamp_lv(ScoreBMS::Lamp lamp)
{
    switch (lamp)
    {
    case ScoreBMS::Lamp::NOPLAY: return BarLampType::NOPLAY;
    case ScoreBMS::Lamp::FAILED: return BarLampType::FAILED;
    case ScoreBMS::Lamp::ASSIST: return BarLampType::ASSIST_EASY;
    case ScoreBMS::Lamp::EASY: return BarLampType::EASY;
    case ScoreBMS::Lamp::NORMAL: return BarLampType::NORMAL;
    case ScoreBMS::Lamp::HARD: return BarLampType::HARD;
    case ScoreBMS::Lamp::EXHARD: return BarLampType::EXHARD;
    case ScoreBMS::Lamp::FULLCOMBO: return BarLampType::FULLCOMBO;
    case ScoreBMS::Lamp::PERFECT:
    case ScoreBMS::Lamp::MAX: return BarLampType::PERFECT; // TODO(rustbell): separate max lamp?
    }
    lunaticvibes::assert_failed("score_to_bar_lamp_lv");
};

bool SpriteBarEntry::update(const lunaticvibes::Time& time)
{
    for (auto& s : sBodyOff)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sBodyOn)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sTitle)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sLevel)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sLamp)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sRank)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sRivalWinLose)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sRivalLampSelf)
        if (s)
            s->setHideInternal(true);
    for (auto& s : sRivalLampRival)
        if (s)
            s->setHideInternal(true);
    if (sFlash)
        sFlash->setHideInternal(true);

    const std::shared_lock l(gSelectContext._mutex);
    const auto& list = gSelectContext.entries;
    if (!list.empty())
    {
        size_t listidx = gSelectContext.selectedEntryIndex + index;
        if (listidx < gSelectContext.highlightBarIndex)
            listidx += list.size() * ((gSelectContext.highlightBarIndex - listidx) / list.size() + 1);
        listidx -= gSelectContext.highlightBarIndex;
        listidx %= list.size();

        _draw = true;

        drawLevel = false;
        drawRank = false;
        drawLamp = false;
        drawRival = false;
        drawRivalLampSelf = false;
        drawRivalLampRival = false;
        drawBodyType = 0;
        drawTitleType = 0;
        drawLevelType = 0;
        drawLampType = 0;
        drawRankType = 0;
        drawRivalType = 0;
        drawRivalLampSelfType = 0;
        drawRivalLampRivalType = 0;

        auto [pEntry, pScore] = list[listidx];
        drawBodyOn = (index == gSelectContext.highlightBarIndex);

        // check new song
        const bool isNewEntry = pEntry->type() == eEntryType::NEW_SONG_FOLDER ||
                                (pEntry->_addTime > static_cast<unsigned long long>(time.norm()) -
                                                        State::get(IndexNumber::NEW_ENTRY_SECONDS));

        auto barTypeIdx = static_cast<size_t>(entry_bar_type(pEntry->type()));
        if (isNewEntry && (BarType)barTypeIdx == BarType::SONG)
        {
            barTypeIdx = (size_t)BarType::NEW_SONG;
        }
        std::shared_ptr<SpriteBase> pBody = nullptr;
        if (!drawBodyOn && sBodyOff[barTypeIdx])
        {
            if (!sBodyOff[barTypeIdx]->update(time))
            {
                _draw = false;
                return false;
            }
            sBodyOff[barTypeIdx]->setHideInternal(false);
            drawBodyType = barTypeIdx;
            pBody = sBodyOff[barTypeIdx];
        }
        if (drawBodyOn && sBodyOn[barTypeIdx])
        {
            if (!sBodyOn[barTypeIdx]->update(time))
            {
                _draw = false;
                return false;
            }
            sBodyOn[barTypeIdx]->setHideInternal(false);
            drawBodyType = barTypeIdx;
            pBody = sBodyOn[barTypeIdx];
        }
        if (pBody == nullptr)
        {
            _draw = false;
            return false;
        }

        const auto& parentRenderParam = pBody->_current;
        _current.rect = parentRenderParam.rect;
        _current.color = parentRenderParam.color;
        _current.angle = parentRenderParam.angle;

        drawTitle = false;
        drawTitleType = size_t(isNewEntry ? BarTitleType::NEW_SONG : BarTitleType::NORMAL);
        if (sTitle[drawTitleType])
        {
            sTitle[drawTitleType]->update(time);
            sTitle[drawTitleType]->setHideInternal(false);
            drawTitle = true;
        }

        drawFlash = gSelectContext.highlightBarIndex == index;
        if (drawFlash && sFlash)
        {
            sFlash->update(time);
            sFlash->setHideInternal(false);
        }

        if ((BarType)barTypeIdx == BarType::SONG || (BarType)barTypeIdx == BarType::NEW_SONG ||
            (BarType)barTypeIdx == BarType::SONG_RIVAL)
        {
            std::shared_ptr<ChartFormatBase> pf;
            if (pEntry->type() == eEntryType::SONG || pEntry->type() == eEntryType::RIVAL_SONG)
            {
                pf = std::reinterpret_pointer_cast<EntryFolderSong>(pEntry)->getCurrentChart();
            }
            else if (pEntry->type() == eEntryType::CHART || pEntry->type() == eEntryType::RIVAL_CHART)
            {
                pf = std::reinterpret_pointer_cast<EntryChart>(pEntry)->_file;
            }
            if (pf)
            {
                switch (pf->type())
                {
                case eChartFormat::BMS: {
                    const auto bms = std::reinterpret_pointer_cast<const ChartFormatBMSMeta>(pf);

                    // level
                    if ((size_t)bms->difficulty < sLevel.size() && sLevel[bms->difficulty])
                    {
                        sLevel[bms->difficulty]->update(time);
                        sLevel[bms->difficulty]->setHideInternal(false);
                        drawLevelType = bms->difficulty;
                        drawLevel = true;
                    }

                    auto score = std::reinterpret_pointer_cast<ScoreBMS>(pScore);
                    if (score)
                    {
                        // TODO rival entry has two lamps
                        {
                            auto lampTypeIdx = static_cast<size_t>(score_to_bar_lamp_lv(score->lamp));
                            auto sprite = sLamp[lampTypeIdx];
                            if (sprite == nullptr)
                            {
                                lampTypeIdx = static_cast<size_t>(score_to_bar_lamp_lr2(score->lamp));
                                sprite = sLamp[(size_t)score_to_bar_lamp_lr2(score->lamp)];
                            }
                            if (sprite != nullptr)
                            {
                                sprite->update(time);
                                sprite->setHideInternal(false);
                                drawLampType = lampTypeIdx;
                                drawLamp = true;
                            }
                        }

                        if ((BarType)barTypeIdx == BarType::SONG_RIVAL)
                        {
                            // rank
                            auto t = Option::getRankType(score->rival_rate);
                            switch (t)
                            {
                            case Option::RANK_0: drawRankType = (size_t)BarRankType::MAX; break;
                            case Option::RANK_1: drawRankType = (size_t)BarRankType::AAA; break;
                            case Option::RANK_2: drawRankType = (size_t)BarRankType::AA; break;
                            case Option::RANK_3: drawRankType = (size_t)BarRankType::A; break;
                            case Option::RANK_4: drawRankType = (size_t)BarRankType::B; break;
                            case Option::RANK_5: drawRankType = (size_t)BarRankType::C; break;
                            case Option::RANK_6: drawRankType = (size_t)BarRankType::D; break;
                            case Option::RANK_7: drawRankType = (size_t)BarRankType::E; break;
                            case Option::RANK_8: drawRankType = (size_t)BarRankType::F; break;
                            case Option::RANK_NONE: drawRankType = (size_t)BarRankType::NONE; break;
                            }
                            if (sRank[drawRankType])
                            {
                                sRank[drawRankType]->update(time);
                                sRank[drawRankType]->setHideInternal(false);
                                drawRank = true;
                            }
                            // win/lose/draw
                            if ((size_t)score->rival_win < sRivalWinLose.size() && sRivalWinLose[score->rival_win])
                            {
                                sRivalWinLose[score->rival_win]->update(time);
                                sRivalWinLose[score->rival_win]->setHideInternal(false);
                                drawRivalType = score->rival_win;
                                drawRival = true;
                            }
                            // rival lamp
                            if (drawLamp && sRivalLampSelf[drawRivalLampSelfType])
                            {
                                drawRivalLampSelfType = drawLampType;
                                drawRivalLampSelf = true;
                                sRivalLampSelf[drawRivalLampSelfType]->update(time);
                                sRivalLampSelf[drawRivalLampSelfType]->setHideInternal(false);
                            }
                            // rival lamp
                            const auto rivalLampTypeIdx = static_cast<size_t>(score_to_bar_lamp_lv(score->rival_lamp));
                            if (sRivalLampRival[rivalLampTypeIdx])
                            {
                                sRivalLampRival[rivalLampTypeIdx]->update(time);
                                sRivalLampRival[rivalLampTypeIdx]->setHideInternal(false);
                                drawRivalLampRivalType = rivalLampTypeIdx;
                                drawRivalLampRival = true;
                            }
                        }
                    }
                    break;
                }
                default: break;
                }
            }
        }
        else if ((BarType)barTypeIdx == BarType::COURSE)
        {
            auto score = std::dynamic_pointer_cast<ScoreBMS>(pScore);
            if (score)
            {
                auto lampTypeIdx = static_cast<size_t>(score_to_bar_lamp_lv(score->lamp));
                auto sprite = sLamp[lampTypeIdx];
                if (sprite == nullptr)
                {
                    lampTypeIdx = static_cast<size_t>(score_to_bar_lamp_lr2(score->lamp));
                    sprite = sLamp[(size_t)score_to_bar_lamp_lr2(score->lamp)];
                }
                if (sprite != nullptr)
                {
                    sprite->update(time);
                    sprite->setHideInternal(false);
                    drawLampType = lampTypeIdx;
                    drawLamp = true;
                }
            }
        }
        // TODO folder lamp

        setRectOffsetBarIndex(parentRenderParam.rect.x, parentRenderParam.rect.y);
    }
    else
    {
        _draw = false;
        // drawBodyType = drawTitle = false;
        // drawLevelType = drawRankType = drawRivalType = drawRivalLampSelfType = drawRivalLampRivalType = -1u;
    }
    return _draw;
}

void SpriteBarEntry::setMotionLoopTo(int t)
{
    lunaticvibes::verify_failed("[Sprite] setMotionLoopTo(f) of SpriteBarEntry should not be used");
}

void SpriteBarEntry::setMotionStartTimer(IndexTimer t)
{
    lunaticvibes::verify_failed("[Sprite] setMotionStartTimer(f) of SpriteBarEntry should not be used");
}

void SpriteBarEntry::appendMotionKeyFrame(const MotionKeyFrame& f)
{
    lunaticvibes::verify_failed("[Sprite] appendMotionKeyFrame(f) of SpriteBarEntry should not be used");
}

void SpriteBarEntry::draw() const {}

void SpriteBarEntry::setRectOffsetAnim(float x, float y)
{
    auto adjust_rect = [](RectF& r, float x, float y) {
        r.x += x;
        r.y += y;
    };

    if (drawBodyOn)
        adjust_rect(sBodyOn[drawBodyType]->_current.rect, x, y);
    else
        adjust_rect(sBodyOff[drawBodyType]->_current.rect, x, y);

    if (drawTitle)
    {
        adjust_rect(sTitle[drawTitleType]->_current.rect, x, y);
        sTitle[drawTitleType]->updateTextRect();
    }
    if (drawLevel)
    {
        adjust_rect(sLevel[drawLevelType]->_current.rect, x, y);
        sLevel[drawLevelType]->updateNumberRect();
    }
    if (drawLamp)
        adjust_rect(sLamp[drawLampType]->_current.rect, x, y);
    if (drawRank)
        adjust_rect(sRank[drawRankType]->_current.rect, x, y);
    if (drawRival)
        adjust_rect(sRivalWinLose[drawRivalType]->_current.rect, x, y);
    if (drawRivalLampSelf)
        adjust_rect(sRivalLampSelf[drawRivalLampSelfType]->_current.rect, x, y);
    if (drawRivalLampRival)
        adjust_rect(sRivalLampRival[drawRivalLampRivalType]->_current.rect, x, y);
}

void SpriteBarEntry::setRectOffsetBarIndex(float x, float y)
{
    auto adjust_rect = [](RectF& r, float x, float y) {
        r.x += x;
        r.y += y;
    };

    if (drawTitle)
    {
        adjust_rect(sTitle[drawTitleType]->_current.rect, x, y);
        sTitle[drawTitleType]->updateTextRect();
    }
    if (drawLevel)
    {
        adjust_rect(sLevel[drawLevelType]->_current.rect, x, y);
        sLevel[drawLevelType]->updateNumberRect();
    }
    if (drawFlash)
        adjust_rect(sFlash->_current.rect, x, y);
    if (drawLamp)
        adjust_rect(sLamp[drawLampType]->_current.rect, x, y);
    if (drawRank)
        adjust_rect(sRank[drawRankType]->_current.rect, x, y);
    if (drawRival)
        adjust_rect(sRivalWinLose[drawRivalType]->_current.rect, x, y);
    if (drawRivalLampSelf)
        adjust_rect(sRivalLampSelf[drawRivalLampSelfType]->_current.rect, x, y);
    if (drawRivalLampRival)
        adjust_rect(sRivalLampRival[drawRivalLampRivalType]->_current.rect, x, y);
}

bool SpriteBarEntry::OnClick(int x, int y)
{
    if (!_draw)
        return false;

    if (_current.rect.x <= x && x < _current.rect.x + _current.rect.w && _current.rect.y <= y &&
        y < _current.rect.y + _current.rect.h)
    {
        if (gSelectContext.highlightBarIndex == index)
        {
            gSelectContext.cursorEnterPending = true;
        }
        else
        {
            if (available)
            {
                gSelectContext.cursorClick = index;
            }
            else
            {
                if (gSelectContext.highlightBarIndex > index)
                    gSelectContext.cursorClickScroll = (gSelectContext.highlightBarIndex - index);
                else
                    gSelectContext.cursorClickScroll = -(index - gSelectContext.highlightBarIndex);
            }
        }
        return true;
    }
    return false;
}

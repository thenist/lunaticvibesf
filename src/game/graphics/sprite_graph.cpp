#include "sprite_graph.h"
#include "game/scene/scene_context.h"

#include <shared_mutex>

SpriteLine::SpriteLine(const SpriteLineBuilder& builder) : SpriteStatic(builder)
{
    _type = SpriteTypes::LINE;
    _player = builder.player;
    _ltype = builder.lineType;
    _field_w = builder.canvasW;
    _field_h = builder.canvasH;
    _start = builder.start;
    _end = builder.end;
    _line = builder.lineWeight;
    textColor = builder.color;
}

void SpriteLine::appendPoint(const ColorPoint& c)
{
    _points.push_back(c);
}

void SpriteLine::draw() const
{
    if (isHidden())
        return;

    for (auto& [p1, p2] : _rects)
    {
        _line.draw(p1, p2, textColor * _current.color);
    }
}

void SpriteLine::updateProgress(const lunaticvibes::Time& t)
{
    int duration = _end - _start;
    if (duration > 0)
    {
        long long rt = t.norm() - State::get(motionStartTimer);
        if (rt >= _start)
        {
            _progress = (double)(rt - _start) / duration;
            _progress = std::clamp(_progress, 0.0, 1.0);
        }
        else
        {
            _progress = 0.0;
        }
    }
    else
    {
        _progress = 1.0;
    }
}

void SpriteLine::updateRects()
{
    if (!gPlayContext.ruleset[_player])
        return;

    auto pushRects = [this](const std::array<uint8_t, PlayContextParams::GRAPH_POINT_NUMBER> points, unsigned maxh,
                            auto cond, auto clip) {
        const size_t size = points.size();
        _rects.clear();

        const auto& r = _current.rect;
        auto region = static_cast<size_t>(std::floor(size * _progress));
        if (region == 0)
            return;
        region--;

        for (size_t i = 0; i < region; ++i)
        {
            if (cond(points[i], points[i + 1]))
            {
                auto make_pos = [&](int ii) -> Point {
                    return {r.x + _field_w * (double(ii) / (size - 1)),
                            r.y - _field_h * (double(clip(points[ii])) / maxh)};
                };
                _rects.emplace_back(make_pos(i), make_pos(i + 1));
                if (static_cast<int>(_rects.back().first.x) == static_cast<int>(_rects.back().second.x) &&
                    static_cast<int>(_rects.back().first.y) == static_cast<int>(_rects.back().second.y))
                    _rects.pop_back();
            }
        }
    };

    auto pushRectsF = [this](const std::array<double, PlayContextParams::GRAPH_POINT_NUMBER> points, double maxh) {
        const size_t size = points.size();
        auto cond = [](int, int) { return true; };
        std::vector<std::pair<Point, Point>> tmp;
        const auto& r = _current.rect;
        auto region = static_cast<size_t>(std::floor(size * _progress));
        if (region == 0)
            return;
        region--;

        for (size_t i = 0; i < region; ++i)
        {
            if (cond(points[i], points[i + 1]))
            {
                auto make_pos = [&](int i) -> Point {
                    return {r.x + _field_w * (double(i) / (size - 1)), r.y - _field_h * points[i] / maxh};
                };
                tmp.emplace_back(make_pos(i), make_pos(i + 1));
            }
        }

        _rects.clear();
        for (auto& [p1, p2] : tmp)
        {
            if (int(p1.x) != int(p2.x) || int(p1.y) != int(p2.y))
                _rects.emplace_back(p1, p2);
        }
    };

    switch (_ltype)
    {
    case LineType::GAUGE_F: {
        const auto h = static_cast<int>(gPlayContext.ruleset[_player]->getClearHealth() * 100);
        std::shared_lock l(gPlayContext._mutex);
        const auto& p = gPlayContext.graphGauge[_player];
        pushRects(
            p, 100.0, [h](int val1, int val2) { return val1 <= h || val2 <= h; },
            [h](int val2) { return val2 > h ? h : val2; });
        break;
    }
    case LineType::GAUGE_C: {
        const auto h = static_cast<int>(gPlayContext.ruleset[_player]->getClearHealth() * 100);
        std::shared_lock l(gPlayContext._mutex);
        const auto& p = gPlayContext.graphGauge[_player];
        pushRects(
            p, 100.0, [h](int val1, int val2) { return val1 >= h || val2 >= h; },
            [h](int val1) { return val1 < h ? h : val1; });
        break;
    }
    case LineType::SCORE: {
        std::shared_lock l(gPlayContext._mutex);
        const auto& p = gPlayContext.graphAcc[_player];
        pushRectsF(p, 100.0);
        break;
    }
    case LineType::SCORE_MYBEST: {
        if (gPlayContext.ruleset[PLAYER_SLOT_MYBEST])
        {
            std::shared_lock l(gPlayContext._mutex);
            const auto& p = gPlayContext.graphAcc[PLAYER_SLOT_MYBEST];
            pushRectsF(p, 100.0);
        }
        break;
    }
    case LineType::SCORE_TARGET: {
        std::shared_lock l(gPlayContext._mutex);
        const auto& p = gPlayContext.graphAcc[PLAYER_SLOT_TARGET];
        pushRectsF(p, 100.0);
        break;
    }
    }
}

bool SpriteLine::update(const lunaticvibes::Time& t)
{
    if (SpriteStatic::update(t))
    {
        if (_current.blend == BlendMode::NONE)
            _current.color.a = 255;

        switch (_ltype)
        {
        case LineType::GAUGE_F:
            _current.color.r = 0;
            _current.color.b = 0;
            break;

        case LineType::GAUGE_C:
            switch (gPlayContext.mods[_player].gauge)
            {
            case PlayModifierGaugeType::EXHARD:
            case PlayModifierGaugeType::DEATH:
            case PlayModifierGaugeType::GRADE_HARD:
            case PlayModifierGaugeType::GRADE_DEATH: _current.color.g = _current.color.r; break;
            default: _current.color.g = 0; break;
            }
            _current.color.b = 0;
            break;

        case LineType::SCORE:
            _current.color.r = 0;
            _current.color.g = 0;
            break;

        case LineType::SCORE_MYBEST:
            _current.color.r = 0;
            _current.color.b = 0;
            break;

        case LineType::SCORE_TARGET:
            _current.color.g = 0;
            _current.color.b = 0;
            break;
        }

        _line._width = _current.rect.w;

        updateProgress(t);
        updateRects();
        return true;
    }
    return false;
}

#pragma once

#include "sprite.h"

#include <game/graphics/sprite_imagetext_charmapping.h>

#include <string>
#include <utility>
#include <vector>

class SpriteImageText : public SpriteText
{
protected:
    std::vector<std::shared_ptr<Texture>> _textures;
    CharMappingList* _chrList;
    // Screen height, for readmes/helpfiles.
    float _maxHeight;
    unsigned textHeight;
    unsigned _firstLine;
    int _margin;

private:
    std::vector<std::pair<char32_t, RectF>> _drawListOrig;
    std::vector<std::pair<char32_t, RectF>> _drawList;
    std::string _textBuf;
    std::u32string _textU32Buf;
    Rect _drawRect;

public:
    struct SpriteImageTextBuilder : SpriteTextBuilder
    {
        std::vector<std::shared_ptr<Texture>> charTextures;
        CharMappingList* charMappingList = nullptr;
        int height = 0;
        int margin = 0;

        [[nodiscard]] std::shared_ptr<SpriteImageText> build() const
        {
            return std::make_shared<SpriteImageText>(*this);
        }
    };

public:
    SpriteImageText() = delete;
    SpriteImageText(const SpriteImageTextBuilder& builder);
    ~SpriteImageText() override = default;

    // Inject after construction.
    void setMaxHeight(float maxHeight) { _maxHeight = maxHeight; };

public:
    void updateTextRect() override;
    bool update(const lunaticvibes::Time& t) override;
    void draw() const override;

private:
    void updateTextTexture(const std::string& text, unsigned first_line);
};

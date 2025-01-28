#include "game/skin/skin_lr2.h"
#include "gmock/gmock.h"

#include <string>
#include <utility>
#include <vector>

#include <common/str_utils.h>

class mock_SkinLR2 : public SkinLR2
{
public:
    mock_SkinLR2(Path p, int loadMode = 0)
        : SkinLR2(std::make_shared<lunaticvibes::SkinLr2SharedData>(), std::move(p), loadMode)
    {
    }
    ~mock_SkinLR2() override = default;

    const std::vector<element>& getDrawQueue() { return drawQueue; }
};

TEST(tLR2Skin, IF1)
{
    std::shared_ptr<mock_SkinLR2> ps = nullptr;
    ASSERT_NO_THROW(ps = std::make_shared<mock_SkinLR2>("lr2skin/if1.lr2skin"));
    ASSERT_EQ(ps->isLoaded(), true);

    ASSERT_EQ(ps->getDrawQueue().size(), 1);
    const auto v = ps->getDrawQueue();
    EXPECT_EQ(v[0].dstOpt, std::vector{dst_option(1)});
}

TEST(tLR2Skin, IF2)
{
    std::shared_ptr<mock_SkinLR2> ps = nullptr;
    ASSERT_NO_THROW(ps = std::make_shared<mock_SkinLR2>("lr2skin/if2.lr2skin"));
    ASSERT_EQ(ps->isLoaded(), true);

    ASSERT_EQ(ps->getDrawQueue().size(), 3);
    const auto v = ps->getDrawQueue();
    EXPECT_EQ(v[0].dstOpt, std::vector{dst_option(1)});
    EXPECT_EQ(v[1].dstOpt, std::vector{dst_option(3)});
    EXPECT_EQ(v[2].dstOpt, std::vector{dst_option(4)});
}

TEST(tLR2Skin, IF3)
{
    std::shared_ptr<mock_SkinLR2> ps = nullptr;
    ASSERT_NO_THROW(ps = std::make_shared<mock_SkinLR2>("lr2skin/if3.lr2skin"));
    ASSERT_EQ(ps->isLoaded(), true);

    ASSERT_EQ(ps->getDrawQueue().size(), 2);
    const auto v = ps->getDrawQueue();
    EXPECT_EQ(v[0].dstOpt, std::vector{dst_option(3)});
    EXPECT_EQ(v[1].dstOpt, std::vector{dst_option(5)});
}

TEST(tLR2Skin, IF4)
{
    std::shared_ptr<mock_SkinLR2> ps = nullptr;
    ASSERT_NO_THROW(ps = std::make_shared<mock_SkinLR2>("lr2skin/if4.lr2skin"));
    ASSERT_EQ(ps->isLoaded(), true);

    ASSERT_EQ(ps->getDrawQueue().size(), 1);
    const auto v = ps->getDrawQueue();
    EXPECT_EQ(v[0].dstOpt, std::vector{dst_option(1)});
}

TEST(tLR2Skin, IfStatementTrailingSingleDigit)
{
    std::shared_ptr<mock_SkinLR2> ps = nullptr;
    ASSERT_NO_THROW(ps = std::make_shared<mock_SkinLR2>("lr2skin/if5.lr2skin"));
    ASSERT_EQ(ps->isLoaded(), true);

    ASSERT_EQ(ps->getDrawQueue().size(), 3);
    const auto v = ps->getDrawQueue();
    EXPECT_EQ(v[0].dstOpt, std::vector{dst_option(1)});
    EXPECT_EQ(v[1].dstOpt, (std::vector{dst_option(1), dst_option(4)}));
    EXPECT_EQ(v[2].dstOpt, std::vector{dst_option(1)});
}

TEST(tLR2Skin, IF6)
{
    std::shared_ptr<mock_SkinLR2> ps = nullptr;
    ASSERT_NO_THROW(ps = std::make_shared<mock_SkinLR2>("lr2skin/if6.lr2skin"));
    ASSERT_EQ(ps->isLoaded(), true);

    ASSERT_EQ(ps->getDrawQueue().size(), 1);
    const auto v = ps->getDrawQueue();
    EXPECT_EQ(v[0].dstOpt, std::vector{dst_option(2)});
}

TEST(tLR2Skin, HelpFileParsed)
{
    mock_SkinLR2 s{"lr2skin/helpfile.lr2skin"};
    ASSERT_EQ(s.isLoaded(), true);

    static const std::vector<std::string> helpFiles{
        "「とわ」とわ\n", "(file error)", "「とわ」とわ\n", "(file error)", "(file error)",
        "(file error)",   "(file error)", "(file error)",   "(file error)", "UTF-8\nテスト\n",
        // No tenth.
    };

    EXPECT_EQ(s.getHelpFiles(), helpFiles);
}

TEST(tLR2Skin, CustomFile)
{
    mock_SkinLR2 s{"lr2skin/customfile.lr2skin"};
    ASSERT_EQ(s.isLoaded(), true);
    ASSERT_EQ(s.getCustomizeOptionCount(), 1);
    {
        const auto& opt = s.getCustomizeOptionInfo(0);
        EXPECT_EQ(opt.id, 0);
        EXPECT_EQ(opt.displayName, "テスト");
        EXPECT_EQ(opt.internalName, "FILE_テスト");
        EXPECT_EQ(opt.defaultEntry, 0);
        EXPECT_EQ(lunaticvibes::join(',', opt.entries), "1,2,RANDOM");
    }
}

#include "replays_layer.hpp"
#include "nodes.hpp"
#include "replay_system.hpp"
#include <filesystem>
#include <fmt/core.h>
#include "log.hpp"
#include <functional>
#include <deque>
#include "nodes/infinite-list.hpp"
#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include <fts_fuzzy_match.h>
#include <date_utils.hpp>
#include "nodes/selection-menu.hpp"
#include "nodes/dropdown-node.hpp"
#include "nodes/settings-layer.hpp"

static constexpr float widget_width = 350;
static constexpr float widget_height = 65;

static constexpr float search_bar_height = 40;

static constexpr float total_height = widget_height * 3.f + search_bar_height;

bool ReplaysLayer::init() {
    if (!CCLayer::init()) return false;
    auto& rs = ReplaySystem::get_instance();
    for (const auto& entry : std::filesystem::directory_iterator(rs.get_replays_path())) {
        const auto& path = entry.path();
        if (std::filesystem::is_regular_file(path) && path.extension() == ".replay") {
            replays.push_back(Replay::load(path.string()));
        }
    }

    const auto win_size = CCDirector::sharedDirector()->getWinSize();

    addChild(
        NodeFactory<CCSprite>::start("GJ_gradientBG.png")
        .setAnchorPoint(ccp(0, 0))
        .setOpacity(200)
        .setColor(ccc3(100, 100, 255))
        .with([&](auto* node) {
            const auto size = node->getScaledContentSize();
            node->setScaleX(win_size.width / size.width);
            node->setScaleY(win_size.height / size.height);
        })
        .setZOrder(-99)
    );

    static constexpr auto list_height = total_height - search_bar_height;

    this->list = InfiniteListLayer::create(widget_width, list_height, widget_height, [&](size_t index) {
        return make_widget(index);
    }, replays.size());

    // infinitelistlayer is anchored on the bottom left
    // maybe figure out a way to have it centered on the center?
    list->setPosition(win_size.width / 2.f - widget_width / 2.f, win_size.height / 2.f - total_height / 2.f);
    addChild(list);

    // search bar
    {
        const float y = win_size.height / 2.f + total_height / 2.f - search_bar_height / 2.f;
        static constexpr float bar_width = widget_width * 0.8f;
        static constexpr float extra_space = 35;

        addChild(
            NodeFactory<CCLayerColor>::start(
                ccColor4B { 40, 40, 40, 50 },
                widget_width, search_bar_height
            )
            .setZOrder(2)
            .setPosition(-widget_width / 2.f + win_size.width / 2.f, -search_bar_height / 2.f + y)
        );


        auto* text_input = TextInputNode::create(CCSize(bar_width - extra_space, 30.f), 1.f, "Search replays", "bigFont.fnt", true);
        text_input->input_node->setLabelPlaceholderScale(0.45f);
        text_input->input_node->setLabelPlaceholderColor(ccc3(180, 180, 180));
        text_input->setPosition(win_size.width / 2.f - extra_space / 2.f, y);
        text_input->input_node->setMaxLabelScale(0.5f);
        text_input->setZOrder(3);
        // TODO: fuzzy search into filtered_replays
        // and make gen_widget care abt it
        text_input->callback = [&](TextInputNode* input) {
            this->filter = input->get_value();
            this->filtered_replays.clear();
            if (this->filter.empty()) {
                this->list->size = this->replays.size();
            } else {
                for (auto& replay : this->replays) {
                    int score;
                    if (fts::fuzzy_match(this->filter.c_str(), fmt::format("{} {}%", replay.level_name, int(replay.died_at)).c_str(), score)) {
                        this->filtered_replays.push_back(replay);
                    }
                }
                this->list->size = this->filtered_replays.size();
            }
            this->list->reset();
        };
        addChild(text_input);
    }


    const auto gen_table_borders = [](CCNode* node, const CCPoint pos, const CCSize size, const int z) {
        auto* spr = CCSprite::createWithSpriteFrameName("GJ_table_side_001.png");
        const auto side_height = spr->getContentSize().height;
        spr->release();

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_side_001.png"))
            .setPosition(ccp(pos.x - size.width / 2.f - 9.f, pos.y))
            .setScale(1.f, size.height / side_height)
            .setZOrder(z)
        );

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_side_001.png"))
            .setPosition(ccp(pos.x + size.width / 2.f + 9.f, pos.y))
            .setScale(-1.f, size.height / side_height)
            .setZOrder(z)
        );

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_bottom_001.png"))
            .setPosition(ccp(pos.x, pos.y - size.height / 2.f))
            .setAnchorPoint(ccp(0.5f, 0.81f))
            .setZOrder(z)
        );

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_top_001.png"))
            .setPosition(ccp(pos.x, pos.y + size.height / 2.f))
            .setAnchorPoint(ccp(0.5f, 0.21f))
            .setZOrder(z)
        );
    };

    gen_table_borders(this, win_size / 2.f, {350, 65 * 3 + 40.f}, 4);

    auto btn = gd::CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png"), this, menu_selector(ReplaysLayer::on_settings));
    auto menu = CCMenu::create();
    menu->setPosition(win_size - ccp(25.f, 25.f));
    menu->addChild(btn);
    this->addChild(menu);

    setKeypadEnabled(true);
    return true;
}


CCNode* ReplaysLayer::make_widget(size_t index) {
    if (index >= replays.size()) return nullptr;

    const Replay& replay = this->filter.empty() ? replays[index] : this->filtered_replays[index];

    auto* node = CCNode::create();
    node->setZOrder(1);

    const unsigned char bg_color = index % 2 == 0 ? 50 : 35;

    // background
    node->addChild(
        NodeFactory<CCLayerColor>::start(
            ccColor4B { bg_color, bg_color, bg_color, 100 },
            widget_width, widget_height
        )
        .setZOrder(-5)
        .setPosition(-widget_width / 2.f, -widget_height / 2.f)
    );

    auto menu = CCMenu::create();
    node->addChild(menu);
    menu->setPosition(0, 0);

    menu->addChild(
        NodeFactory<gd::CCMenuItemSpriteExtra>::start(
            NodeFactory<gd::ButtonSprite>::start("View", 50, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.8f)
                .setScale(0.8f).end(),
            this, menu_selector(ReplaysLayer::on_view)
        )
        .setPosition(ccp(widget_width / 2.f - 40.f, 0.f))
        .setTag(index)
    );

    // level name
    node->addChild(
        NodeFactory<CCLabelBMFont>::start(replay.level_name.c_str(), "bigFont.fnt")
        .setAnchorPoint(ccp(0, 0))
        .setPosition(ccp(-widget_width / 2.f + 60.f, 0.f))
        .with([](auto* node) { node->limitLabelWidth(180.f, 0.8f, 0.2f); })
    );

    // percent
    node->addChild(
        NodeFactory<CCLabelBMFont>::start(fmt::format("{}%", int(replay.died_at)).c_str(), "goldFont.fnt")
        .setAnchorPoint(ccp(0.f, 1.f))
        .setPosition(ccp(-widget_width / 2.f + 60.f, 0.f))
        .setScale(0.6f)
    );


    // why are you always zero..
    // auto online_levels = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    // auto* level = online_levels[fmt::format("{}", replay.level_id)]; // gotta love robtop
    // if (level) {
    //     logln("{}: {}", replay.level_name, level->difficulty);
    // }

    // difficulty icon
    node->addChild(
        NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("diffIcon_06_btn_001.png"))
        .setPosition(ccp(-widget_width / 2.f + 30.f, 0.f))
        .setScale(1.1f)
    );

    // i forgot to add created_at to the replay file..
    // // timestamp
    // const auto date = date_info::from_point(replay.created_at);
    // const auto timestamp = fmt::format("{}-{:02}-{:02} {:02}:{:02}", date.year, date.month, date.day, date.hour, date.minutes, date.seconds);

    // node->addChild(
    //     NodeFactory<CCLabelBMFont>::start(timestamp.c_str(), "chatFont.fnt")
    //     .setPosition(ccp(widget_width / 2.f - 3.f, -widget_height / 2.f + 3.f))
    //     .setScale(0.6f)
    //     .setAnchorPoint(ccp(1.f, 0.f))
    //     .setColor(ccc3(220, 220, 220))
    // );

    return node;
}

void ReplaysLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void ReplaysLayer::on_view(CCObject* sender) {
    auto btn = static_cast<gd::CCMenuItemSpriteExtra*>(sender);
    const auto index = static_cast<size_t>(btn->getTag());
    if (index >= replays.size()) return;

    const Replay& replay = this->filter.empty() ? replays[index] : filtered_replays[index];
    
    auto online_levels = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    auto lvl = online_levels[std::to_string(replay.level_id)];
    // sorry levels with empty level string but its a much easier way to check
    if (lvl && !lvl->levelNotDownloaded && !lvl->levelString.empty()) {
        auto& rs = ReplaySystem::get_instance();
        rs.get_replay() = replay;
        rs.toggle_playing();

        auto layer = gd::PlayLayer::create(lvl);
        layer->m_isTestMode = true; // ez safe mode
        rs.update_status_label();
        
        CCDirector::sharedDirector()->pushScene(NodeFactory<CCScene>::start().addChild(layer));
    } else {
        gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "Level not found or not downloaded, unable to play replay")->show();
    }
}

class OverlayWrapperLayer : public CCLayerColor {
public:
    static auto create(CCNode* child) {
        auto obj = new OverlayWrapperLayer;
        if (obj->init(child))
            obj->autorelease();
        else
            CC_SAFE_DELETE(obj);
        return obj;
    }

    bool init(CCNode* child) {
        if (!CCLayerColor::initWithColor(ccc4(0, 0, 0, 100))) return false;

        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchEnabled(true);

        this->setKeypadEnabled(true);
        this->setKeyboardEnabled(true);

        this->addChild(child);

        return true;
    }

    // might not be needed.. but oh well
    // this is only for swallowing touches
    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        return true;
    }

    void keyBackClicked() override {
        this->removeFromParentAndCleanup(true);
    }

    void show() {
        auto* scene = CCDirector::sharedDirector()->getRunningScene();
        const int z = scene->getHighestChildZ() + 1;
        scene->addChild(this, z);
    }
};

void ReplaysLayer::on_settings(CCObject*) {
    auto* settings = SettingsLayer::create(350.f);
    auto& rs = ReplaySystem::get_instance();

    settings->add_setting("Record replays", settings->checkmark(rs.record_replays, [&rs](CCNode* node) {
        rs.record_replays = !static_cast<gd::CCMenuItemToggler*>(node)->isOn();
    }));

    // settings->add_setting("Replay type", settings->picker(rs.get_default_type() == ReplayType::XPOS, { "Frame", "XPos" }, [&rs](auto* picker) {
    //     rs.set_default_type(picker->get_index() == 0 ? ReplayType::FRAME : ReplayType::XPOS);
    // }));

    settings->add_setting("Always save completions", settings->checkmark(false, [](CCNode* node) {
    }));

    settings->add_setting("Save every attempt", settings->checkmark(false, [](CCNode* node) {
    }));

    // number input, not checkmark
    settings->add_setting("Replay buffer size", settings->checkmark(false, [](CCNode* node) {
    }));

    OverlayWrapperLayer::create(settings)->show();
}

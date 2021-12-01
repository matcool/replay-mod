#include "replays_layer.hpp"
#include "nodes.hpp"
#include "replay_system.hpp"
#include <filesystem>
#include <fmt/core.h>
#include "log.hpp"

bool ReplaysLayer::init() {
    if (!CCLayer::init()) return false;
    auto& rs = ReplaySystem::get_instance();
    auto saved_levels = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    for (const auto& entry : std::filesystem::directory_iterator(rs.get_replays_path())) {
        const auto& path = entry.path();
        if (std::filesystem::is_regular_file(path) && path.extension() == ".replay") {
            replays.push_back(Replay::load(path.string()));
        }
    }

    gen_widgets();
    
    const auto win_size = CCDirector::sharedDirector()->getWinSize();
    auto menu = CCMenu::create();
    menu->setPosition(ccp(0, 0));
    menu->addChild(
        NodeFactory<gd::CCMenuItemSpriteExtra>::start(
            CCSprite::createWithSpriteFrameName("edit_upBtn_001.png"), this, menu_selector(ReplaysLayer::on_scroll_arrow)
        )
        .setTag(0)
        .setPosition(ccp(win_size.width / 2.f, win_size.height - 35.f))
    );

    menu->addChild(
        NodeFactory<gd::CCMenuItemSpriteExtra>::start(
            CCSprite::createWithSpriteFrameName("edit_downBtn_001.png"), this, menu_selector(ReplaysLayer::on_scroll_arrow)
        )
        .setTag(1)
        .setPosition(ccp(win_size.width / 2.f, 35.f))
    );

    addChild(menu);
    
    setKeypadEnabled(true);
    return true;
}

void ReplaysLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void ReplaysLayer::on_see(CCObject* sender) {
    auto btn = cast<gd::CCMenuItemSpriteExtra*>(sender);
    auto i = btn->getTag();
    auto& rs = ReplaySystem::get_instance();
    if (i < 0 || i >= replays.size()) return;
    auto& replay = replays[i];
    auto d = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    auto lvl = d[std::to_string(replay.level_id)];
    if (lvl && !lvl->levelNotDownloaded) {
        rs.get_replay() = replay;
        rs.toggle_playing();
        auto layer = gd::PlayLayer::create(lvl);
        layer->m_isTestMode = true; // ez safe mode
        rs.update_status_label();
        CCDirector::sharedDirector()->pushScene(NodeFactory<CCScene>::start().addChild(layer));
    }
}

void ReplaysLayer::gen_widgets() {
    if (widgets)
        widgets->removeAllChildrenWithCleanup(true);
    else {
        widgets = CCNode::create();
        widgets->setPosition(ccp(0, 0));
        addChild(widgets);
    }

    const auto win_size = CCDirector::sharedDirector()->getWinSize();
    float y = win_size.height - 90;
    
    for (auto& [i, replay] : enumerate(replays)) {
        if (i < scroll || i > scroll + 2) continue;
        auto widget = CCNode::create();
        widget->addChild(NodeFactory<extension::CCScale9Sprite>::start("GJ_square01.png").setContentSize(ccp(400, 60)));
        widget->setPosition(280, y);
        auto menu = CCMenu::create();
        widget->addChild(menu);
        menu->setPosition(0, 0);

        menu->addChild(
            NodeFactory<gd::CCMenuItemSpriteExtra>::start(
                gd::ButtonSprite::create("see", 50, false, "goldFont.fnt", "GJ_button_01.png", 0, 1.f), this, menu_selector(ReplaysLayer::on_see))
            .setPosition(152, 0)
            .setTag(i)
        );

        widget->addChild(
            NodeFactory<CCLabelBMFont>::start(fmt::format("{}\n{}%", replay.level_name, int(replay.died_at)).c_str(), "bigFont.fnt")
            .setPosition(-185, 0)
            .setScale(0.4f)
            .setAnchorPoint(ccp(0, 0.5f))
            .setAlignment(kCCTextAlignmentLeft)
        );

        widgets->addChild(widget);

        y -= 70.f;
    }
}

template <class T, class U, class V>
inline T clamp(const T value, const U lower, const V upper) {
    return value > upper ? upper : value < lower ? lower : value;
}

void ReplaysLayer::on_scroll_arrow(CCObject* sender) {
    bool down = cast<CCNode*>(sender)->getTag() == 1;
    auto new_scroll = clamp(int(scroll) + (down * 2 - 1), 0, int(replays.size()) - 1);
    if (scroll != new_scroll) {
        scroll = new_scroll;
        logln("scrolling to {}", scroll);
        gen_widgets();
    }
}
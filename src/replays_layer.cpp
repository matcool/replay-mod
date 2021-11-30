#include "replays_layer.hpp"
#include "nodes.hpp"
#include "replay_system.hpp"
#include <filesystem>

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

    for (auto& [i, replay] : enumerate(replays)) {
        auto widget = CCNode::create();
        widget->addChild(NodeFactory<extension::CCScale9Sprite>::start("GJ_square01.png").setContentSize(ccp(400, 60)));
        widget->setPosition(280, 35 + i * 70);
        auto menu = CCMenu::create();
        widget->addChild(menu);
        menu->setPosition(0, 0);

        menu->addChild(
            NodeFactory<gd::CCMenuItemSpriteExtra>::start(
                gd::ButtonSprite::create("see", 50, false, "goldFont.fnt", "GJ_button_01.png", 0, 1.f), this, menu_selector(ReplaysLayer::on_see))
            .setPosition(152, 0)
            .setTag(i));

        widget->addChild(
            NodeFactory<CCLabelBMFont>::start(replay.level_name.c_str(), "bigFont.fnt")
            .setPosition(-185, 0)
            .setScale(0.55f)
            .setAnchorPoint(ccp(0, 0.5f)));

        addChild(widget);
    }
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
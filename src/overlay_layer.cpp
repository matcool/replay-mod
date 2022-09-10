#include "overlay_layer.hpp"
#include "replay_system.hpp"
#include <nfd.h>
#include <sstream>
#include "nodes.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <date_utils.hpp>

bool OverlayLayer::init() {
    if (!initWithColor({ 0, 0, 0, 105 })) return false;

    setZOrder(20);

    auto win_size = CCDirector::sharedDirector()->getWinSize();
    auto& rs = ReplaySystem::get_instance();
        
    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    addChild(menu);

    // CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);
    // registerWithTouchDispatcher();
    
    menu->addChild(NodeFactory<gd::CCMenuItemSpriteExtra>::start(
        NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png")).setScale(0.75f).end(),
        this, menu_selector(OverlayLayer::close_btn_callback))
        .setPosition(ccp(18, win_size.height - 18))
    );
    
    float y = win_size.height - 50.f;
    for (auto& [i, replay] : enumerate(rs.temp_replays)) {
        addChild(
            NodeFactory<CCLabelBMFont>::start(fmt::format("{}\n{}%", replay->level_name, int(replay->died_at)).c_str(), "bigFont.fnt")
            .setPosition(ccp(40, y))
            .setAlignment(kCCTextAlignmentLeft)
            .setAnchorPoint(ccp(0, 0.5))
            .setScale(0.5f)
        );

        // FIXME: saving two at the same time crashes :3

        menu->addChild(
            NodeFactory<gd::CCMenuItemSpriteExtra>::start(
                NodeFactory<gd::ButtonSprite>::start("save", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 0.5f).end(),
                this, menu_selector(OverlayLayer::on_save)
            )
            .setTag(i)
            .setPosition(ccp(250, y))
        );

        y -= 50.f;
    }

    setKeypadEnabled(true);
    setTouchEnabled(true);

    return true;
}


void OverlayLayer::FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2) {

}

void OverlayLayer::on_save(CCObject* sender) {
    auto& rs = ReplaySystem::get_instance();
    auto& vec = rs.temp_replays;
    size_t index = static_cast<CCNode*>(sender)->getTag();

    rs.save_replay(*vec[index].get());

    vec.erase(vec.begin() + index);
    static_cast<CCNode*>(sender)->removeFromParentAndCleanup(true);
}

void OverlayLayer::keyBackClicked() {
    // CCDirector::sharedDirector()->getTouchDispatcher()->decrementForcePrio(2);
    removeFromParentAndCleanup(true);
}

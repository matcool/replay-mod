#include "selection-menu.hpp"
#include <cocos-ext.h>
#include "../nodes.hpp"
#include "../log.hpp"

using namespace cocos2d;

SelectionMenuPopup* SelectionMenuPopup::create(const std::vector<std::string>& options, std::function<void(size_t index)> cb) {
    auto obj = new SelectionMenuPopup;
    if (obj && obj->init(options, cb)) {
        obj->autorelease();
    } else {
        CC_SAFE_DELETE(obj);
    }
    return obj;
}

bool SelectionMenuPopup::init(const std::vector<std::string>& options, std::function<void(size_t index)> callback) {
    if (!CCLayer::init()) return false;

    this->callback = callback;

    const float width = 140;
    const float height = 30;
    const CCSize size(width, height * options.size());
    auto bg = make_factory(extension::CCScale9Sprite::create("GJ_square05.png"))
        .setContentSize(size * 2.f)
        .setScale(0.5f)
        .setZOrder(-2)
        .setAnchorPoint(ccp(0.5, 1.0))
        .end();
    this->popup_rect = make_rect(bg->getPosition(), bg->getScaledContentSize());
    this->popup_rect.origin = this->popup_rect.origin - (bg->getAnchorPoint() * this->popup_rect.size);
    this->addChild(bg);
    this->addChild(
        make_factory(CCLayerColor::create(ccc4(0, 0, 0, 100), width - 4, height - 4))
        .setPosition(-width / 2.f + 2.f, -height - height * 0.f + 2.f)
        .setZOrder(-1)
        .with([this](auto* node) { this->highlight_bar = node; })
    );
    // this->setPosition(winSize.width / 2 + 95, winSize.height / 2 + 40);
    for (const auto [i, option] : enumerate(options)) {
        this->addChild(
            make_factory(CCLabelBMFont::create(option.c_str(), "bigFont.fnt"))
            .setPosition(0.f, i * -height - height / 2.f)
            .limitLabelWidth(width, 0.6f, 0.0f)
        );
    }
    this->setContentSize(size);

    // makes regsiterWithTouchDispatcher use targeted dispatcher with swallow touches set to true
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchPriority(-300);

    this->setTouchEnabled(true);

    this->scheduleUpdate();

    return true;
}

bool SelectionMenuPopup::ccTouchBegan(CCTouch* touch, CCEvent*) {
    auto actual_rect = this->popup_rect;
    actual_rect.origin = actual_rect.origin + this->getPosition();
    if (actual_rect.containsPoint(touch->getLocation())) {
        const auto rel_pos = actual_rect.size.height - (touch->getLocation().y - actual_rect.origin.y);
        const float height = 30;
        const auto index = static_cast<int>(rel_pos / height);
        this->callback(index);
    }
    this->removeFromParentAndCleanup(true);
    return true;
}

void SelectionMenuPopup::update(float) {
    const float width = 140;
    const float height = 30;
    const auto mouse_pos = get_mouse_pos() - this->getPosition() + ccp(width / 2.f, 0);
    const auto index = static_cast<int>(mouse_pos.y / -height);
    if (mouse_pos.x < 0 || mouse_pos.x > width || mouse_pos.y > 0 || mouse_pos.y < -this->popup_rect.size.height) {
        this->highlight_bar->setVisible(false);
    } else {
        this->highlight_bar->setVisible(true);
        this->highlight_bar->setPosition(-width / 2.f + 2.f, -height - height * index + 2.f);
    }
}
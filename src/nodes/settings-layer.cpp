#include "settings-layer.hpp"
#include <gd.h>
#include "dropdown-node.hpp"
#include "../nodes.hpp"

using namespace cocos2d;

SettingsLayer* SettingsLayer::create(float width) {
    auto obj = new SettingsLayer;
    obj->init(width);
    obj->autorelease();
    return obj;
}

bool SettingsLayer::init(float width) {
    if (!CCLayer::init()) return false;

    const auto win_size = CCDirector::sharedDirector()->getWinSize();

    // addChild(
    //     make_factory(CCSprite::create("GJ_gradientBG.png"))
    //     .setAnchorPoint(ccp(0, 0))
    //     .setOpacity(200)
    //     .setColor(ccc3(100, 100, 255))
    //     .with([&](auto* node) {
    //         const auto size = node->getScaledContentSize();
    //         node->setScaleX(win_size.width / size.width);
    //         node->setScaleY(win_size.height / size.height);
    //     })
    //     .setZOrder(-99)
    // );

    this->width = width;

    y = win_size.height - 50.f;
    prev_y = y;

    // this->setKeypadEnabled(true);

    return true;
}

// void keyBackClicked() override {
//     CCDirector::sharedDirector()->popScene();
// }

void SettingsLayer::add_setting(const std::string& label, CCNode* node) {
    auto label_node = make_factory(CCLabelBMFont::create(label.c_str(), "bigFont.fnt"))
        .setAnchorPoint(ccp(0.f, 0.5f))
        .limitLabelWidth(width / 2.f, 0.6f, 0.f)
        .end();
    const float height = std::max(node->getScaledContentSize().height, label_node->getScaledContentSize().height);
    const float pad = 30.f;
    if (prev_y != y && prev_y - y < height) {
        const auto space = prev_y - y;
        y -= height / 2.f - space + pad;
    }
    const auto win_size = CCDirector::sharedDirector()->getWinSize();
    label_node->setPosition(win_size.width / 2.f - width / 2.f + 5.f, y);
    this->addChild(label_node);
    node->setPosition(win_size.width / 2.f + width / 2.f - node->getScaledContentSize().width / 2.f - 5.f, y);
    this->addChild(node);
    prev_y = y;
    y -= height / 2.f + pad;
}

CCNode* SettingsLayer::checkmark(bool value, SettingCallback callback) {
    return make_factory(CCMenu::create())
        .setPosition(0, 0)
        .addChild(make_factory(gd::CCMenuItemToggler::create(
                CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
                CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
                this, menu_selector(SettingsLayer::on_checkmark)
            ))
            .with([&](auto* node) {
                node->toggle(value);
                m_callbacks[node] = callback;
            })
            .end()
        )
        .with(set_menu_size)
        .end();
}

void SettingsLayer::on_checkmark(CCObject* sender) {
    auto* node = static_cast<CCNode*>(sender);
    m_callbacks[node](node);
}

CCNode* SettingsLayer::picker(size_t index, const std::vector<std::string>& options, std::function<void(DropdownNode*)> callback) {
    auto* node = DropdownNode::create(options, callback);
    node->pick(index);

    auto* menu = make_factory(CCMenu::create())
    .setPosition(0, 0)
    .addChild(
        make_factory(CCMenuItemSprite::create(node, node, this, menu_selector(SettingsLayer::on_picker)))
        .with([&](auto* item) {
            m_callbacks[item] = [node](CCNode*) {
                node->open();
            };
        })
        .end()
    )
    .with(set_menu_size)
    .end();

    // for some reason ccmenuitemsprite sets the anchor point of the node
    node->setAnchorPoint(ccp(-0.5, -0.5));

    return menu;
}

void SettingsLayer::on_picker(CCObject* sender) {
    // arg is ignored
    m_callbacks[static_cast<CCNode*>(sender)](nullptr);
}
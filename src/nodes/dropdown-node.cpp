#include "dropdown-node.hpp"
#include "../nodes.hpp"
#include "selection-menu.hpp"

using namespace cocos2d;

DropdownNode* DropdownNode::DropdownNode::create(const std::vector<std::string>& options, Callback callback) {
    auto obj = new DropdownNode;
    obj->init(options, callback);
    obj->autorelease();
    return obj;
}

bool DropdownNode::init(const std::vector<std::string>& options, Callback callback) {
    if (!CCNode::init()) return false;

    m_options = options;
    m_callback = callback;

    const float width = 140;
    const float height = 30;
    const float space = 30;

    make_factory(this)
    .setContentSize(CCSize(width, height))
    .addChild(
        make_factory(extension::CCScale9Sprite::create("GJ_square05.png"))
        .setContentSize(CCSize(width * 2, height * 2))
        .setScale(0.5f)
        .setZOrder(-1)
        .end()
    )
    .addChild(
        make_factory(CCLabelBMFont::create("", "bigFont.fnt"))
        .limitLabelWidth(width - space - 5, 0.6f, 0.f)
        .setPosition(-space / 2.f + 3.f, 0)
        .with([this](auto* node) { m_label = node; })
        .end()
    )
    .addChild(
        make_factory(CCSprite::createWithSpriteFrameName("edit_downBtn_001.png"))
        .setPosition(width / 2.f - space / 2.f, 0.f)
        .setScale(0.8f, 0.7f)
        .end()
    );

    return true;
}

void DropdownNode::pick(size_t index) {
    if (index >= m_options.size()) {
        // logln("You baffoon");
    } else {
        m_index = index;
        m_label->setString(m_options[index].c_str());
    }
}

void DropdownNode::open() {
    const float height = 30;
    const auto world_pos = this->convertToWorldSpace(this->getPosition());
    CCDirector::sharedDirector()->getRunningScene()->addChild(
        make_factory(SelectionMenuPopup::create(m_options, [this](size_t index) {
            this->pick(index);
            m_callback(this);
        }))
        .setZOrder(999)
        .setPosition(world_pos - ccp(0, height / 2.f))
    );
}

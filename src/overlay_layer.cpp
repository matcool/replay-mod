#include "overlay_layer.hpp"
#include "replay_system.hpp"
#include <nfd.h>
#include <sstream>

bool OverlayLayer::init() {
    if (!initWithColor({ 0, 0, 0, 105 })) return false;

    setZOrder(20);

    auto win_size = CCDirector::sharedDirector()->getWinSize();
    auto rs = ReplaySystem::get_instance();
        
    auto menu = CCMenu::create();
    menu->setPosition({0, win_size.height});
    addChild(menu);

    this->registerWithTouchDispatcher();
    CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

    auto sprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    sprite->setScale(0.75f);
    
    auto btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::close_btn_callback));
    btn->setPosition({18, -18});
    menu->addChild(btn);

    sprite = CCSprite::create("GJ_button_01.png");
    sprite->setScale(0.72f);
    
    btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::on_record));
    btn->setPosition({35, -50});
    menu->addChild(btn);

    auto label = CCLabelBMFont::create("Record", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5});
    label->setScale(0.8f);
    label->setPosition({55, win_size.height - 50});
    addChild(label);

    btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::on_play));
    btn->setPosition({35, -85});
    menu->addChild(btn);

    label = CCLabelBMFont::create("Play", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5});
    label->setScale(0.8f);
    label->setPosition({55, win_size.height - 85});
    addChild(label);

    sprite = CCSprite::create("GJ_button_02.png");
    sprite->setScale(0.72f);

    btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::on_save));
    btn->setPosition({win_size.width - 35, -50});
    menu->addChild(btn);

    label = CCLabelBMFont::create("Save", "bigFont.fnt");
    label->setAnchorPoint({1, 0.5});
    label->setScale(0.8f);
    label->setPosition({win_size.width - 55, win_size.height - 50});
    addChild(label);

    btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::on_load));
    btn->setPosition({win_size.width - 35, -85});
    menu->addChild(btn);

    label = CCLabelBMFont::create("Load", "bigFont.fnt");
    label->setAnchorPoint({1, 0.5});
    label->setScale(0.8f);
    label->setPosition({win_size.width - 55, win_size.height - 85});
    addChild(label);

    sprite = CCSprite::create("square02b_001.png");
    sprite->setColor({0, 0, 0});
    sprite->setOpacity(69);
    sprite->setPosition({110, win_size.height - 115});
    sprite->setScaleX(0.825f);
    sprite->setScaleY(0.325f);
    sprite->setZOrder(-1);
    addChild(sprite);

    m_fps_input = gd::CCTextInputNode::create("fps", nullptr, "bigFont.fnt", 100.f, 100.f);
    m_fps_input->setString(std::to_string(static_cast<int>(rs->get_default_fps())).c_str());
    m_fps_input->setLabelPlaceholderColor({200, 200, 200});
    m_fps_input->setLabelPlaceholerScale(0.5f);
    m_fps_input->setMaxLabelScale(0.7f);
    m_fps_input->setMaxLabelLength(0);
    m_fps_input->setAllowedChars("0123456789");
    m_fps_input->setMaxLabelLength(10);
    m_fps_input->setPosition({110, win_size.height - 115});
    addChild(m_fps_input);

    label = CCLabelBMFont::create("FPS:", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5f});
    label->setScale(0.7f);
    label->setPosition({20, win_size.height - 115});
    addChild(label);

    m_replay_info = CCLabelBMFont::create("", "chatFont.fnt");
    m_replay_info->setAnchorPoint({0, 1});
    m_replay_info->setPosition({20, win_size.height - 133});
    update_info_text();
    addChild(m_replay_info);

    setKeypadEnabled(true);
    setTouchEnabled(true);

    return true;
}

void OverlayLayer::update_info_text() {
    auto rs = ReplaySystem::get_instance();
    auto& replay = rs->get_replay();
    std::stringstream stream;
    stream << "Current Replay:\nFPS: " << replay.get_fps() << "\nActions: " << replay.get_actions().size();
    m_replay_info->setString(stream.str().c_str());
}

void OverlayLayer::_update_default_fps() {
    auto text = m_fps_input->getString();
    if (text[0])
        ReplaySystem::get_instance()->set_default_fps(std::stof(text));
}

void OverlayLayer::on_record(CCObject*) {
    _update_default_fps();
    ReplaySystem::get_instance()->toggle_recording();
    update_info_text();
}

void OverlayLayer::on_play(CCObject*) {
    ReplaySystem::get_instance()->toggle_playing();
}

void OverlayLayer::on_save(CCObject*) {
    nfdchar_t* path = nullptr;
    auto result = NFD_SaveDialog("replay", nullptr, &path);
    if (result == NFD_OKAY) {
        std::string s_path(path);
        // why doesnt it just add the extension for me
        ReplaySystem::get_instance()->get_replay().save(s_path + ".replay");
        gd::FLAlertLayer::create(nullptr, "Info", "Ok", nullptr, "Replay saved.")->show();
        free(path);
    }
}

void OverlayLayer::on_load(CCObject*) {
    nfdchar_t* path = nullptr;
    auto result = NFD_OpenDialog("replay", nullptr, &path);
    if (result == NFD_OKAY) {
        ReplaySystem::get_instance()->get_replay() = Replay::load(path);
        update_info_text();
        gd::FLAlertLayer::create(nullptr, "Info", "Ok", nullptr, "Replay loaded.")->show();
        free(path);
    }
}

void OverlayLayer::keyBackClicked() {
    _update_default_fps();
    gd::FLAlertLayer::keyBackClicked();
}
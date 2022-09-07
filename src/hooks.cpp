#include "hooks.hpp"
#include "replay_system.hpp"
#include "overlay_layer.hpp"
#include <chrono>
#include <matdash/minhook.hpp>
#include "replays_layer.hpp"
#include "log.hpp"

// yes these are global, too lazy to store them in replaysystem or smth
// not like theyre used anywhere else atm
bool g_disable_render = false;
float g_left_over = 0.f;

void Hooks::CCScheduler_update(CCScheduler* self, float dt) {
    auto& rs = ReplaySystem::get_instance();
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    if (play_layer && (rs.is_playing() || rs.is_recording())) {
        const auto fps = rs.get_replay().get_fps();
        auto speedhack = self->getTimeScale();

        const float target_dt = 1.f / fps / speedhack;

        if (!rs.real_time_mode || (!play_layer->unk2EC && dt > 0.1f)) // isNotFrozen
            return orig<&CCScheduler_update, Thiscall>(self, target_dt);

        // todo: find ways to disable more render stuff
        g_disable_render = false;

        unsigned times = static_cast<int>((dt + g_left_over) / target_dt);
        if (dt == 0.f)
            return orig<&CCScheduler_update, Thiscall>(self, target_dt);
        auto start = std::chrono::high_resolution_clock::now();
        for (unsigned i = 0; i < times; ++i) {
            // if (i == times - 1)
            //     g_disable_render = false;
            orig<&CCScheduler_update, Thiscall>(self, target_dt);
            using namespace std::literals;
            if (std::chrono::high_resolution_clock::now() - start > 33.333ms) {
                times = i + 1;
                break;
            }
        }
        g_left_over += dt - target_dt * times;
    } else {
        orig<&CCScheduler_update, Thiscall>(self, dt);
    }
}

void Hooks::PlayLayer::update(gd::PlayLayer* self, float dt) {
    auto& rs = ReplaySystem::get_instance();
    if (rs.is_playing()) rs.handle_playing();
    orig<&update, Thiscall>(self, dt);
}


bool _player_button_handler(bool hold, bool button) {
    if (gd::GameManager::sharedState()->getPlayLayer()) {
        auto& rs = ReplaySystem::get_instance();
        if (rs.is_playing()) return true;
        rs.record_action(hold, button);
    }
    return false;
}

void Hooks::PlayLayer::pushButton(gd::PlayLayer* self, int idk, bool button) {
    if (_player_button_handler(true, button)) return;
    orig<&pushButton>(self, idk, button);
}

void Hooks::PlayLayer::releaseButton(gd::PlayLayer* self, int idk, bool button) {
    if (_player_button_handler(false, button)) return;
    orig<&releaseButton>(self, idk, button);
}

void Hooks::PlayLayer::resetLevel(gd::PlayLayer* self) {
    orig<&resetLevel>(self);
    ReplaySystem::get_instance().on_reset();
}


void Hooks::PlayLayer::pauseGame(gd::PlayLayer* self, bool idk) {
    auto addr = cast<void*>(gd::base + 0x20D43C);
    auto& rs = ReplaySystem::get_instance();
    if (rs.is_recording())
        rs.record_action(false, true, false);

    bool should_patch = rs.is_playing();
    if (should_patch)
        patch(addr, {0x83, 0xC4, 0x04, 0x90, 0x90});
    
    orig<&pauseGame>(self, idk);

    if (should_patch)
        patch(addr, {0xe8, 0x2f, 0x7b, 0xfe, 0xff});
}

void Hooks::PlayLayer::levelComplete(gd::PlayLayer* self) {
    orig<&levelComplete>(self);
    // TODO: support start pos
    if (!self->m_isPracticeMode && !self->m_isTestMode) {
        ReplaySystem::get_instance().reset_state(true);
    }
}

void _on_exit_level() {
    auto& rs = ReplaySystem::get_instance();
    rs.reset_state();
}

void Hooks::PlayLayer::onQuit(gd::PlayLayer* self) {
    _on_exit_level();
    orig<&onQuit>(self);
}

void Hooks::PauseLayer_onEditor(gd::PauseLayer* self, CCObject* idk) {
    _on_exit_level();
    orig<&PauseLayer_onEditor>(self, idk);
}

bool Hooks::PauseLayer_init(gd::PauseLayer* self) {
    if (!orig<&PauseLayer_init>(self)) return false;
    // dont add the button to the pause menu if its playing
    if (ReplaySystem::get_instance().is_playing()) return true;

    auto win_size = CCDirector::sharedDirector()->getWinSize();
    
    auto menu = CCMenu::create();
    menu->setPosition(35, win_size.height - 40.f);
    self->addChild(menu);
    
    auto sprite = CCSprite::create("GJ_button_01.png");
    sprite->setScale(0.72f);
    auto btn = gd::CCMenuItemSpriteExtra::create(sprite, self, menu_selector(OverlayLayer::open_btn_callback));
    menu->addChild(btn);
    
    auto label = CCLabelBMFont::create("Save replays", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5});
    label->setScale(0.5f);
    label->setPositionX(20);
    menu->addChild(label);
    return true;
}

void Hooks::PlayLayer::updateVisiblity(gd::PlayLayer* self) {
    if (!g_disable_render)
        orig<&updateVisiblity>(self);
}

bool MenuLayer_init(gd::MenuLayer* self) {
    if (!orig<&MenuLayer_init>(self)) return false;

    auto btn = gd::CCMenuItemSpriteExtra::create(CCSprite::create("GJ_button_01.png"), self, menu_selector(ReplaysLayer::open_btn_callback));
    auto menu = CCMenu::create();
    menu->setPosition(200, 200);
    menu->addChild(btn);
    self->addChild(menu);

    return true;
}

bool Hooks::PlayLayer::init(gd::PlayLayer* self, gd::GJGameLevel* lvl) {
    if (!orig<&init>(self, lvl)) return false;
    auto& rs = ReplaySystem::get_instance();
    if (!rs.is_playing()) {
        if (rs.record_replays)
            rs.start_recording();
        else if (rs.is_recording())
            rs.stop_recording();
    }
    logln("rs state is {}, {}", rs.is_recording(), rs.is_playing());
    return true;
}

void PlayerObject_playerDestroyed(gd::PlayerObject* self, bool idk) {
    orig<&PlayerObject_playerDestroyed>(self, idk);
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    if (play_layer && self != play_layer->m_player2) {
        auto& rs = ReplaySystem::get_instance();
        logln("rs state is {}, {}", rs.is_recording(), rs.is_playing());
        if (rs.is_recording()) {
            // TODO: support start pos
            // prob not going to support checkpoints, but maybe
            if (!play_layer->m_isPracticeMode && !play_layer->m_isTestMode) {
                rs.push_current_replay();
            }
        } else if (rs.is_playing()) {
            auto director = CCDirector::sharedDirector();
            // i have to run this next frame as the action is created after the call to playerDestroyed
            constexpr const auto selector = [](CCObject* self, float d) {
                auto play_layer = cast<gd::PlayLayer*>(self);
                // stop the auto retry action
                play_layer->stopActionByTag(16);
                constexpr const auto call_selector = [](CCObject* self) {
                    auto play_layer = cast<gd::PlayLayer*>(self);
                    // auto label = CCLabelBMFont::create("u suck", "bigFont.fnt");
                    // label->setPosition(200, 200);
                    // play_layer->addChild(label);
                    CCDirector::sharedDirector()->getOpenGLView()->showCursor(true);
                    gd::FLAlertLayer::create(nullptr, "info", "ok", nullptr, "replay died")->show();
                };
                auto action = CCCallFunc::create(play_layer, union_cast<SEL_CallFunc>(thiscall<void(CCObject*)>::wrap<call_selector>));
                play_layer->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(0.5f), action));
            };
            director->getScheduler()->scheduleSelector(union_cast<SEL_SCHEDULE>(thiscall<void(CCObject*, float)>::wrap<selector>),
                play_layer, 0.f, 0, 0.f, false);
        }
    }
}

// alk touch dispatcher fixes (just undoing what rob did Lol)
class CCTouchDispatcherFix : public CCTouchDispatcher {
public:
    void addTargetedDelegate_(CCTouchDelegate* delegate, int prio, bool swallowsTouches) {
        this->setForcePrio(false);
        if (m_pTargetedHandlers->count() > 0) {
            auto* handler = static_cast<CCTouchHandler*>(m_pTargetedHandlers->objectAtIndex(0));
            prio = handler->getPriority() - 2;
        }

        orig<&CCTouchDispatcherFix::addTargetedDelegate_>(this, delegate, prio, swallowsTouches);
    }
    void incrementForcePrio_(int) {
        this->setForcePrio(false);
    }
    void decrementForcePrio_(int) {
        this->setForcePrio(false);
    }
};

auto cocos(const char* symbol) {
    static auto mod = GetModuleHandleA("libcocos2d.dll");
    return GetProcAddress(mod, symbol);
}

void Hooks::init() {
    add_hook<&CCScheduler_update, Thiscall>(cocos("?update@CCScheduler@cocos2d@@UAEXM@Z"));
    // add_hook<&CCKeyboardDispatcher_dispatchKeyboardMSG>(cocos("?dispatchKeyboardMSG@CCKeyboardDispatcher@cocos2d@@QAE_NW4enumKeyCodes@2@_N@Z"));

    add_hook<&PlayLayer::init>(gd::base + 0x1fb780);
    add_hook<&PlayLayer::update, Thiscall>(gd::base + 0x2029C0);

    add_hook<&PlayLayer::pushButton>(gd::base + 0x111500);
    add_hook<&PlayLayer::releaseButton>(gd::base + 0x111660);
    add_hook<&PlayLayer::resetLevel>(gd::base + 0x20BF00);

    add_hook<&PlayLayer::pauseGame>(gd::base + 0x20D3C0);

    add_hook<&PlayLayer::levelComplete>(gd::base + 0x1FD3D0);
    add_hook<&PlayLayer::onQuit>(gd::base + 0x20D810);
    add_hook<&PauseLayer_onEditor>(gd::base + 0x1E60E0);

    add_hook<&PlayLayer::updateVisiblity>(gd::base + 0x205460);

    add_hook<&PauseLayer_init>(gd::base + 0x1E4620);

    add_hook<&MenuLayer_init>(gd::base + 0x1907b0);
    add_hook<&PlayerObject_playerDestroyed>(gd::base + 0x1efaa0);

    add_hook<&CCTouchDispatcherFix::addTargetedDelegate_>(cocos("?addTargetedDelegate@CCTouchDispatcher@cocos2d@@QAEXPAVCCTouchDelegate@2@H_N@Z"));
    add_hook<&CCTouchDispatcherFix::incrementForcePrio_>(cocos("?incrementForcePrio@CCTouchDispatcher@cocos2d@@QAEXH@Z"));
    add_hook<&CCTouchDispatcherFix::decrementForcePrio_>(cocos("?decrementForcePrio@CCTouchDispatcher@cocos2d@@QAEXH@Z"));
}
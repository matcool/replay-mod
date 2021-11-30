#include "replay_system.hpp"
#include "hooks.hpp"

void ReplaySystem::record_action(bool hold, bool player1, bool flip) {
    if (is_recording()) {
        auto gm = gd::GameManager::sharedState();
        auto play_layer = gm->getPlayLayer();
        auto is_two_player = play_layer->m_levelSettings->m_twoPlayerMode;
        player1 ^= flip && gm->getGameVariable("0010");
        Action action;
        action.hold = hold;
        action.player2 = is_two_player && !player1;
        if (replay.get_type() == ReplayType::XPOS)
            action.x = play_layer->m_player1->m_position.x;
        else if (replay.get_type() == ReplayType::FRAME)
            action.frame = get_frame();
        replay.add_action(action);
    }
}

void ReplaySystem::play_action(const Action& action) {
    auto gm = gd::GameManager::sharedState();
    auto flip = gm->getGameVariable("0010");
    if (action.hold) orig<&Hooks::PlayLayer::pushButton>(gm->getPlayLayer(), 0, !action.player2 ^ flip);
    else orig<&Hooks::PlayLayer::releaseButton>(gm->getPlayLayer(), 0, !action.player2 ^ flip);
}

unsigned ReplaySystem::get_frame() {
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    if (play_layer != nullptr) {
       return static_cast<unsigned>(play_layer->m_time * replay.get_fps()) + frame_offset;
    }
    return 0;
}

void ReplaySystem::update_frame_offset() {
    // if there is no last checkpoint then it should default to 0
    frame_offset = practice_fixes.get_last_checkpoint().frame;
}

void ReplaySystem::on_reset() {
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    frame_offset = 0;
    action_index = 0;
    if (is_recording()) {
        replay = Replay(1.f / float(CCDirector::sharedDirector()->getAnimationInterval()), default_type);
        if (play_layer->m_player1->m_isHolding) {
            record_action(true, true, false);
            // TODO: this makes some funky levels impossible lol
            orig<&Hooks::PlayLayer::releaseButton>(play_layer, 0, true);
            orig<&Hooks::PlayLayer::pushButton>(play_layer, 0, true);
            play_layer->m_player1->m_hasJustHeld = true;
        }
    }
}

void ReplaySystem::push_current_replay() {
    // TODO: maybe move this over to the replay class
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    replay.died_at = play_layer->m_hasCompletedLevel ? 100.f : play_layer->m_player1->m_position.x / play_layer->m_levelLength * 100.f;
    replay.level_name = play_layer->m_level->levelName;
    replay.created_at = std::chrono::system_clock::now();
    temp_replays.push_back(std::make_shared<Replay>(replay));
    if (temp_replays.size() > 5)
        temp_replays.erase(temp_replays.begin());
}

void ReplaySystem::handle_playing() {
    if (is_playing()) {
        auto x = gd::GameManager::sharedState()->getPlayLayer()->m_player1->m_position.x;
        auto& actions = replay.get_actions();
        Action action;
        if (replay.get_type() == ReplayType::XPOS) {
            while (action_index < actions.size() && x >= (action = actions[action_index]).x) {
                play_action(action);
                ++action_index;
            }
        } else {
            while (action_index < actions.size() && get_frame() >= (action = actions[action_index]).frame) {
                play_action(action);
                ++action_index;
            }
        }
    }
}

// why here out of all places? idk

constexpr int STATUS_LABEL_TAG = 10032;

auto _create_status_label(CCLayer* layer) {
    auto label = CCLabelBMFont::create("", "chatFont.fnt");
    label->setTag(STATUS_LABEL_TAG);
    label->setAnchorPoint({0, 0});
    label->setPosition({5, 5});
    label->setZOrder(999);
    layer->addChild(label);
    return label;
}

void ReplaySystem::update_status_label() {
    return;
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    if (play_layer) {
        auto label = cast<CCLabelBMFont*>(play_layer->getChildByTag(STATUS_LABEL_TAG));
        if (!label)
            label = _create_status_label(play_layer);
        switch (state) {
            case NOTHING:
                label->setString("");
                if (recorder.m_recording && (!recorder.m_until_end || from_offset<bool>(play_layer, 0x4BD)))
                    recorder.stop();
                break;
            case RECORDING:
                label->setString("Recording");
                break;
            case PLAYING:
                label->setString(showcase_mode ? "" : "Playing");
                break;
        }
    } else if (recorder.m_recording) {
        recorder.stop();
    }
}

void ReplaySystem::reset_state(bool save) {
    if (save && is_recording()) push_current_replay();
    state = NOTHING;
    frame_advance = false;
    update_frame_offset();
    update_status_label();
}

std::filesystem::path ReplaySystem::get_replays_path() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(NULL), buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path() / "matreplays";
}
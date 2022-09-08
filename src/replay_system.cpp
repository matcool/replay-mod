#include "replay_system.hpp"
#include "hooks.hpp"
#include "log.hpp"
#include <date_utils.hpp>
#include <fmt/format.h>

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

float get_active_fps_limit() {
	auto* app = cocos2d::CCApplication::sharedApplication();
	if (app->getVerticalSyncEnabled()) {
		static const float refresh_rate = [] {
			DEVMODEA device_mode;
			memset(&device_mode, 0, sizeof(device_mode));
			device_mode.dmSize = sizeof(device_mode);
			device_mode.dmDriverExtra = 0;

			if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &device_mode) == 0) {
				return 60.f;
			} else {
				return static_cast<float>(device_mode.dmDisplayFrequency);
			}
		}();
		return refresh_rate;
	} else {
		return static_cast<float>(1.0 / cocos2d::CCDirector::sharedDirector()->getAnimationInterval());
	}
}

void ReplaySystem::start_recording() {
    logln("start_recording {}", state);
    state = RECORDING;
    replay = Replay(get_active_fps_limit(), default_type);
    update_status_label();
}

void ReplaySystem::on_reset() {
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    frame_offset = 0;
    action_index = 0;
    if (is_recording()) {
        replay = Replay(get_active_fps_limit(), default_type);
        logln("replay is {}", replay.get_fps());
        if (play_layer->m_player1->m_isHolding) {
            record_action(true, true, false);
            // TODO: this makes some funky levels impossible lol
            orig<&Hooks::PlayLayer::releaseButton>(play_layer, 0, true);
            orig<&Hooks::PlayLayer::pushButton>(play_layer, 0, true);
            play_layer->m_player1->m_hasJustHeld = true;
        }
    } else if (!is_playing()) {
        if (record_replays) {
            // really make sure its recording if record_replays is true
            start_recording();
        } else {
            // make sure its not recording otherwise :-)
            stop_recording();
        }
    }
}

void ReplaySystem::push_current_replay() {
    // TODO: maybe move this over to the replay class
    logln("pushing current replay");
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    replay.died_at = play_layer->m_hasCompletedLevel ? 100.f : play_layer->m_player1->m_position.x / play_layer->m_levelLength * 100.f;
    replay.level_name = play_layer->m_level->levelName;
    replay.created_at = std::chrono::system_clock::now();
    if (save_every_attempt || (auto_save_completions && play_layer->m_hasCompletedLevel)) {
        save_replay(replay);
    } else {
        temp_replays.push_back(std::make_shared<Replay>(replay));
        if (temp_replays.size() > replay_buffer_size)
            temp_replays.erase(temp_replays.begin());
    }
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
                break;
            case RECORDING:
                label->setString("Recording");
                break;
            case PLAYING:
                label->setString(showcase_mode ? "" : "Playing");
                break;
        }
    }
}

void ReplaySystem::reset_state(bool save) {
    if (save && is_recording()) push_current_replay();
    state = NOTHING;
    update_status_label();
}

std::filesystem::path ReplaySystem::get_replays_path() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(nullptr), buffer, MAX_PATH);
    const auto path = std::filesystem::path(buffer).parent_path() / "matreplays";
    // ensure folder exists
    std::filesystem::create_directory(path);
    return path;
}

void ReplaySystem::save_replay(Replay& replay) {
    auto date = date_info::from_point(replay.created_at);
    auto filename = fmt::format(FMT_STRING("{} {}-{:02}-{:02} {:02}-{:02}-{:02} {}.replay"),
        replay.level_name, date.year, date.month, date.day, date.hour, date.minutes, date.seconds, int(replay.died_at));
    const auto path = get_replays_path();
    replay.save((path / filename).string());
}
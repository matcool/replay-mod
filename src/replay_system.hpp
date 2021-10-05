#pragma once
#include "replay.hpp"
#include "practice_fixes.hpp"
#include "recorder.hpp"
#include <vector>
#include <memory>

// this is a rly bad name
enum RSState {
    NOTHING,
    RECORDING,
    PLAYING
};

class ReplaySystem {
    float default_fps;

    Replay replay;
    RSState state = NOTHING;
    ReplayType default_type;

    size_t action_index = 0;

    PracticeFixes practice_fixes;

    bool frame_advance = false;

    ReplaySystem() : default_fps(240.f), replay(default_fps), default_type(replay.get_type()) {}

    unsigned frame_offset = 0;
public:
    std::vector<std::shared_ptr<Replay>> temp_replays;

    static auto& get_instance() {
        static ReplaySystem instance;
        return instance;
    }

    inline auto& get_replay() { return replay; }
    inline auto get_default_fps() { return default_fps; }
    inline void set_default_fps(float fps) { default_fps = fps; }

    inline auto get_default_type() { return default_type; }
    inline void set_default_type(ReplayType type) { default_type = type; }

    inline bool is_playing() { return state == PLAYING; }
    inline bool is_recording() { return state == RECORDING; }

    void update_frame_offset();
    void update_status_label();

    void toggle_playing() {
        state = is_playing() ? NOTHING : PLAYING;
        update_frame_offset();
        update_status_label();
    }
    void toggle_recording() {
        state = is_recording() ? NOTHING : RECORDING;
        if (!is_recording()) frame_advance = false;
        else replay = Replay(1.f / float(CCDirector::sharedDirector()->getAnimationInterval()), default_type);
        update_frame_offset();
        update_status_label();
    }

    void reset_state() {
        state = NOTHING;
        frame_advance = false;
        update_frame_offset();
        update_status_label();
    } 

    void record_action(bool hold, bool player1, bool flip = true);
    void play_action(const Action& action);

    void on_reset();

    void handle_playing();

    inline auto& get_practice_fixes() { return practice_fixes; }

    inline bool get_frame_advance() { return frame_advance; }
    inline void set_frame_advance(bool b) { frame_advance = b; }

    unsigned get_frame();

    bool real_time_mode = true; // fuck it we going public
    bool showcase_mode = false;
    Recorder recorder;
};
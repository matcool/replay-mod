#pragma once
#include "replay.hpp"
#include <vector>
#include <memory>
#include <filesystem>
#include "log.hpp"

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

    void update_status_label();

    void toggle_playing() {
        logln("toggle_playing {}", state);
        state = is_playing() ? NOTHING : PLAYING;
        update_status_label();
    }
    void start_recording();
    void stop_recording() {
        logln("stop_recording {}", state);
        state = NOTHING;
    }

    void reset_state(bool save = false);

    void push_current_replay();

    void record_action(bool hold, bool player1, bool flip = true);
    void play_action(const Action& action);

    void on_reset();

    void handle_playing();

    unsigned get_frame();

    std::filesystem::path get_replays_path();

    void save_replay(Replay& replay);

    void new_replay();

    bool real_time_mode = true; // fuck it we going public
    bool showcase_mode = false;

    bool record_replays = true;
    bool auto_save_completions = false;
    bool save_every_attempt = false;

    size_t replay_buffer_size = 5;
};
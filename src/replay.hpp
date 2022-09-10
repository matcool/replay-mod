#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <chrono>

enum ReplayType : std::uint8_t {
    XPOS,
    FRAME
};

using u64 = uint64_t;
using i32 = int32_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

struct Action {
    union {
	    float x;
        u32 frame;
    };
	bool hold;
	bool player2;
};

class Replay {
protected:
    std::vector<Action> actions;
    float fps;
    ReplayType type;
public:
    std::string level_name;
    i32 level_id = 0;
    float died_at = 0;
    std::chrono::time_point<std::chrono::system_clock> created_at;

    i32 acc_id = 0;
    bool is_new_best = false;
    u32 session_attempts = 0;
    u32 session_time = 0;
    u32 star_count = 0;
    u8 star_gain = 0;
    u32 orb_count = 0;
    u16 orb_gain = 0;

    Replay(float fps, ReplayType type = ReplayType::FRAME);

    float get_fps() { return fps; }
    
    void add_action(const Action& action) { actions.push_back(action); }
    auto& get_actions() { return actions; }
    void remove_actions_after(float x);
    void remove_actions_after(unsigned frame);

    auto get_type() { return type; }

    void save(const std::string& path);
    static Replay load(const std::string& path);
};
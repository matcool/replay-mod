#pragma once
#include <vector>
#include <string>
#include <iostream>

enum ReplayType : std::uint8_t {
    XPOS,
    FRAME
};

struct Action {
    union {
	    float x;
        unsigned frame;
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
    int level_id = 0;
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
#pragma once
#include "includes.h"
#include <vector>
#include "replay.hpp"

class ReplaysLayer : public CCLayer {
public:
    std::vector<Replay> replays;
    class InfiniteListLayer* list;
    // great design mat
    std::string filter;
    std::vector<std::reference_wrapper<Replay>> filtered_replays;
    size_t scroll = 0;
    CCNode* widgets = nullptr;

    bool init();
    
    static auto create() {
        auto ret = new ReplaysLayer;
        if (ret && ret->init())
            ret->autorelease();
        else
            CC_SAFE_DELETE(ret);
        return ret;
    }
    
    void open_btn_callback(CCObject*) {
        auto scene = CCScene::create();
        scene->addChild(ReplaysLayer::create());
        CCDirector::sharedDirector()->pushScene(scene);
    }
    
    virtual void keyBackClicked();
    
    void on_view(CCObject*);

    CCNode* make_widget(size_t index);
};

class ReplayEndPopup : public gd::FLAlertLayer {
public:
};
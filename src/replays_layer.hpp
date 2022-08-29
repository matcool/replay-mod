#pragma once
#include "includes.h"
#include <vector>
#include "replay.hpp"

class ReplaysLayer : public CCLayer {
public:
    std::vector<Replay> replays;
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
    
    void gen_widgets();

    void on_view(CCObject*);
    void on_scroll_arrow(CCObject*);

    CCNode* make_widget(size_t index);
};

class ReplayEndPopup : public gd::FLAlertLayer {
public:
};
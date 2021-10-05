#pragma once
#include "includes.h"

class ReplaysLayer : public CCLayer {
public:
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
    void on_see(CCObject*);
};
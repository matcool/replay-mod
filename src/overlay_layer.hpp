#pragma once
#include "includes.h"

class OverlayLayer : public CCLayerColor, public CCTextFieldDelegate, public gd::FLAlertLayerProtocol {
    gd::CCTextInputNode* m_fps_input;
    CCLabelBMFont* m_replay_info;
    gd::CCMenuItemToggler* m_x_pos_toggle;
    gd::CCMenuItemToggler* m_frame_toggle;

public:
    static auto create() {
        auto node = new OverlayLayer;
        if (node && node->init()) {
            node->autorelease();
        } else {
            CC_SAFE_DELETE(node);
        }
        return node;
    }

    virtual bool init();

    void open_btn_callback(CCObject*) {
        auto node = create();
        CCDirector::sharedDirector()->getRunningScene()->addChild(node);
    }

    void close_btn_callback(CCObject*) {
        keyBackClicked();
    }

    void on_save(CCObject*);

    virtual void keyBackClicked();
    virtual void keyDown(enumKeyCodes key) {
        // keyDown overwrites keyBackClicked, how fun
        if (key == 27) keyBackClicked();
    }
    virtual void FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2);
};
#pragma once
#include <cocos2d.h>
#include <functional>

class SelectionMenuPopup : public cocos2d::CCLayer {
    cocos2d::CCRect popup_rect;
    std::function<void(size_t index)> callback;
    cocos2d::CCNode* highlight_bar = nullptr;
public:
    static SelectionMenuPopup* create(const std::vector<std::string>& options, std::function<void(size_t index)> callback);

    bool init(const std::vector<std::string>&, std::function<void(size_t index)>);

    void update(float);

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override;

    void keyBackClicked() override;
};
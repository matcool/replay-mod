#pragma once

#include <cocos2d.h>
#include <functional>
#include <string>
#include <vector>

class DropdownNode;

class SettingsLayer : public cocos2d::CCLayer {
public:
    static SettingsLayer* create(float width);

    void add_setting(const std::string& label, cocos2d::CCNode* node);

    using SettingCallback = std::function<void(cocos2d::CCNode*)>;

    cocos2d::CCNode* checkmark(bool value, SettingCallback callback);

    cocos2d::CCNode* picker(size_t index, const std::vector<std::string>& options, std::function<void(DropdownNode*)> callback);

protected:
    float width;
    float y, prev_y;

    bool init(float width);

    std::unordered_map<cocos2d::CCNode*, SettingCallback> m_callbacks;

    void on_checkmark(CCObject* sender);
    void on_picker(CCObject* sender);

    // void keyBackClicked() override;

};
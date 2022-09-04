#pragma once

#include <cocos2d.h>
#include <vector>
#include <string>

class DropdownNode : public cocos2d::CCNode {
public:
    using Callback = std::function<void(DropdownNode*)>;

    static DropdownNode* create(const std::vector<std::string>& options, Callback callback);

    void pick(size_t index);

    void open();

    auto get_index() { return m_index; }
protected:
    bool init(const std::vector<std::string>& options, Callback callback);

    std::vector<std::string> m_options;
    cocos2d::CCLabelBMFont* m_label;
    Callback m_callback;
    size_t m_index = 0;
};
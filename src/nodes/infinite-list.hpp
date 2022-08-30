#pragma once
#include <cocos2d.h>
#include <functional>
#include <deque>

class InfiniteListLayer : public cocos2d::CCLayer {
public:
    static InfiniteListLayer* create(float width, float height, float cell_height, std::function<cocos2d::CCNode*(size_t index)> provider, size_t size);

    size_t size;

    void update_list();
    void reset();

protected:
    bool init(float width, float height, float cell_height, std::function<cocos2d::CCNode*(size_t index)> provider, size_t size);
    void ccTouchesBegan(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override;
    void ccTouchesMoved(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override;
    void scrollWheel(float dy, float) override;
    void ccTouchesEnded(cocos2d::CCSet* touches, cocos2d::CCEvent* event) override;
    void update(float dt) override;
    void setup();

    // should never be less than 0
    float m_scroll_offset = 0.f;
    size_t m_current_index = 0;
    // how big each cell should be
    float m_cell_height;
    std::deque<cocos2d::CCNode*> m_nodes;
    // returns the cell for an index
    std::function<cocos2d::CCNode*(size_t index)> m_cell_provider;
    size_t m_max_cells;
    float m_velocity = 0.f;
    cocos2d::CCClippingNode* m_clipping;
};
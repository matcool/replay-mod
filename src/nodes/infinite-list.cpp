#include "infinite-list.hpp"
#include <algorithm>
#include "../nodes.hpp"

using namespace cocos2d;

InfiniteListLayer* InfiniteListLayer::create(float width, float height, float cell_height, std::function<CCNode*(size_t index)> provider, size_t size) {
    auto obj = new InfiniteListLayer;
    if (obj && obj->init(width, height, cell_height, provider, size)) {
        obj->autorelease();
    } else {
        CC_SAFE_DELETE(obj);
    }    
    return obj;
}

bool InfiniteListLayer::init(float width, float height, float cell_height, std::function<CCNode*(size_t index)> provider, size_t size) {
    if (!CCLayer::init()) return false;

    m_cell_height = cell_height;
    m_cell_provider = provider;
    this->size = size;

    // max number of cells on screen
    const auto max_cells = static_cast<size_t>(height / cell_height) + 1;
    m_max_cells = max_cells;

    m_clipping = CCClippingNode::create(CCLayerColor::create(ccc4(0, 0, 0, 255), width, height));
    this->addChild(m_clipping);

    this->setContentSize({ width, height });
    this->setup();

    this->setTouchEnabled(true);
    this->setMouseEnabled(true);
    return true;
}

static constexpr int INFINITE_LIST_TOUCH_TAG = 4469;

void InfiniteListLayer::ccTouchesBegan(CCSet* touches, CCEvent* event) {
    auto* touch = static_cast<CCTouch*>(touches->anyObject());
    if (!touch) return;
    const auto pos = touch->getLocation();
    if (make_rect(this->getPosition(), this->getContentSize()).containsPoint(pos)) {
        touch->setTag(INFINITE_LIST_TOUCH_TAG);
        this->unscheduleUpdate();
    }
}

void InfiniteListLayer::ccTouchesMoved(CCSet* touches, CCEvent* event) {
    auto* touch = static_cast<CCTouch*>(touches->anyObject());
    if (!touch) return;
    if (touch->getTag() == INFINITE_LIST_TOUCH_TAG) {
        const auto pos = touch->getLocation();
        const auto prev = touch->getPreviousLocation();
        m_velocity = pos.y - prev.y;
        m_scroll_offset += m_velocity;
        this->update_list();
    }
}

void InfiniteListLayer::scrollWheel(float dy, float) {
    m_scroll_offset += dy * 1.5f;
    this->update_list();
}

void InfiniteListLayer::ccTouchesEnded(CCSet* touches, CCEvent* event) {
    auto* touch = static_cast<CCTouch*>(touches->anyObject());
    if (!touch) return;
    if (touch->getTag() == INFINITE_LIST_TOUCH_TAG) {
        // m_velocity set from last time touches moved was called
        this->scheduleUpdate();
    }
}

void InfiniteListLayer::update(float dt) {
    m_scroll_offset += m_velocity * 80.f * dt;
    // this took a lot of brain power to think of
    m_velocity *= powf(0.01f, dt);
    this->update_list();
    if (fabs(m_velocity) <= 0.2f) {
        m_velocity = 0.f;
        this->unscheduleUpdate();
    }
}

void InfiniteListLayer::update_list() {
    if (m_scroll_offset < 0.f) m_scroll_offset = 0.f;

    // TODO: fix being able to skip more than one index at a time

    const auto new_index = static_cast<size_t>(m_scroll_offset / m_cell_height);
    const auto max_index = m_max_cells - 1 > this->size ? 0 : this->size - (m_max_cells - 1);
    if (max_index == 0) {
        m_scroll_offset = 0.f;
    } else if (new_index != m_current_index) {
        const bool went_down = new_index > m_current_index;
        // index of new item to fetch
        const auto new_item_index = went_down ? new_index + m_max_cells - 1 : new_index;
        if (new_item_index >= this->size) {
            m_scroll_offset = max_index * m_cell_height;
        } else {
            // m_nodes shouldnt be empty, if it was then the new item index would be greater than the size (0)
            if (went_down) {
                m_nodes.front()->removeAllChildrenWithCleanup(true);
                m_nodes.pop_front();
            } else {
                m_nodes.back()->removeAllChildrenWithCleanup(true);
                m_nodes.pop_back();
            }

            auto* new_node = m_cell_provider(new_item_index);
            m_clipping->addChild(new_node);
            if (went_down) {
                m_nodes.push_back(new_node);
            } else {
                m_nodes.push_front(new_node);
            }
            m_current_index = new_index;
        }
    }

    const auto size = this->getContentSize();
    const float y = size.height - m_cell_height / 2.f + (m_scroll_offset - m_current_index * m_cell_height);
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i]->setPosition(ccp(
            size.width / 2.f,
            y - m_cell_height * static_cast<float>(i) 
        ));
    }
}

void InfiniteListLayer::setup() {
    m_current_index = 0;
    m_scroll_offset = 0;
    const auto size = this->getContentSize();
    for (size_t i = 0; i < std::min(this->size, m_max_cells); ++i) {
        auto* cell = m_cell_provider(i);
        if (!cell) break;
        m_nodes.push_back(cell);
        m_clipping->addChild(cell);

        cell->setPosition(ccp(size.width / 2.f, size.height - m_cell_height / 2.f - m_cell_height * static_cast<float>(i)));
    }
}

void InfiniteListLayer::reset() {
    m_clipping->removeAllChildrenWithCleanup(true);
    m_nodes.clear();
    this->setup();
}
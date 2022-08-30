#include "replays_layer.hpp"
#include "nodes.hpp"
#include "replay_system.hpp"
#include <filesystem>
#include <fmt/core.h>
#include "log.hpp"
#include <functional>
#include <deque>

static constexpr float widget_width = 350;
static constexpr float widget_height = 65;

static constexpr float search_bar_height = 40;

static constexpr float total_height = widget_height * 3.f + search_bar_height;

CCRect make_rect(const CCPoint& origin, const CCSize& size) {
    return CCRect(origin.x, origin.y, size.width, size.height);
}

class InfiniteListLayer : public CCLayer {
public:
    static auto* create(float width, float height, float cell_height, std::function<CCNode*(size_t index)> provider, size_t size) {
        // FIXME:
        // TODO:
        auto obj = new InfiniteListLayer;
        obj->init(width, height, cell_height, provider, size);
        obj->autorelease();
        return obj;
    }

    float m_scroll_offset = 0.f; // should never be less than 0
    size_t m_current_index = 0;
    float m_cell_height = 100.f; // how big each cell should be
    std::deque<CCNode*> m_nodes;
    std::function<CCNode*(size_t index)> m_cell_provider; // returns the cell for an index, returns nullptr if no more
    size_t m_max_cells;
    float m_velocity = 0.f;
    CCClippingNode* m_clipping;

    size_t size;

    bool init(float width, float height, float cell_height, std::function<CCNode*(size_t index)> provider, size_t size) {
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

    void ccTouchesBegan(CCSet* touches, CCEvent* event) override {
        auto* touch = static_cast<CCTouch*>(touches->anyObject());
        if (!touch) return;
        const auto pos = touch->getLocation();
        if (make_rect(this->getPosition(), this->getContentSize()).containsPoint(pos)) {
            touch->setTag(INFINITE_LIST_TOUCH_TAG);
            this->unscheduleUpdate();
        }
    }

    void ccTouchesMoved(CCSet* touches, CCEvent* event) override {
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

    void scrollWheel(float dy, float) override {
        logln("scroll {}", dy);
        m_scroll_offset += dy * 1.5f;
        this->update_list();
    }

    void ccTouchesEnded(CCSet* touches, CCEvent* event) override {
        auto* touch = static_cast<CCTouch*>(touches->anyObject());
        if (!touch) return;
        if (touch->getTag() == INFINITE_LIST_TOUCH_TAG) {
            // m_velocity set from last time touches moved was called
            this->scheduleUpdate();
        }
    }

    void update(float dt) override {
        m_scroll_offset += m_velocity * 80.f * dt;
        // this took a lot of brain power to think of
        m_velocity *= powf(0.01f, dt);
        this->update_list();
        if (fabs(m_velocity) <= 0.2f) {
            m_velocity = 0.f;
            this->unscheduleUpdate();
        }
    }

    void update_list() {
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

    void setup() {
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

    void reset() {
        m_clipping->removeAllChildrenWithCleanup(true);
        m_nodes.clear();
        this->setup();
    }
};


bool ReplaysLayer::init() {
    if (!CCLayer::init()) return false;
    auto& rs = ReplaySystem::get_instance();
    // auto saved_levels = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    for (const auto& entry : std::filesystem::directory_iterator(rs.get_replays_path())) {
        const auto& path = entry.path();
        if (std::filesystem::is_regular_file(path) && path.extension() == ".replay") {
            replays.push_back(Replay::load(path.string()));
        }
    }

    const auto win_size = CCDirector::sharedDirector()->getWinSize();

    addChild(
        NodeFactory<CCSprite>::start("GJ_gradientBG.png")
        .setAnchorPoint(ccp(0, 0))
        .setOpacity(200)
        .setColor(ccc3(100, 100, 255))
        .with([&](auto* node) {
            const auto size = node->getScaledContentSize();
            node->setScaleX(win_size.width / size.width);
            node->setScaleY(win_size.height / size.height);
        })
        .setZOrder(-99)
    );

    static constexpr auto list_height = total_height - search_bar_height;

    this->list = InfiniteListLayer::create(widget_width, list_height, widget_height, [&](size_t index) {
        return make_widget(index);
    }, replays.size());

    // infinitelistlayer is anchored on the bottom left
    // maybe figure out a way to have it centered on the center?
    list->setPosition(win_size.width / 2.f - widget_width / 2.f, win_size.height / 2.f - total_height / 2.f);
    addChild(list);

    // search bar
    {
        const float y = win_size.height / 2.f + total_height / 2.f - search_bar_height / 2.f;
        static constexpr float bar_width = widget_width * 0.8f;
        static constexpr float extra_space = 35;

        addChild(
            NodeFactory<CCLayerColor>::start(
                ccColor4B { 40, 40, 40, 50 },
                widget_width, search_bar_height
            )
            .setZOrder(2)
            .setPosition(-widget_width / 2.f + win_size.width / 2.f, -search_bar_height / 2.f + y)
        );


        auto* text_input = TextInputNode::create(CCSize(bar_width - extra_space, 30.f), 1.f, "Search replays", "bigFont.fnt", true);
        text_input->input_node->setLabelPlaceholderScale(0.45f);
        text_input->input_node->setLabelPlaceholderColor(ccc3(180, 180, 180));
        text_input->setPosition(win_size.width / 2.f - extra_space / 2.f, y);
        text_input->input_node->setMaxLabelScale(0.5f);
        text_input->setZOrder(3);
        // TODO: fuzzy search into filtered_replays
        // and make gen_widget care abt it
        text_input->callback = [&](TextInputNode* input) {
            // this->filter = input->get_value();
            // this->filtered_replays.clear();
            // for (auto& replay : this->replays) {
            //     // dont look at this next line
            //     if (replay.level_name._Starts_with(this->filter)) {
            //         this->filtered_replays.push_back(replay);
            //     }
            // }
            // this->list->size = this->filtered_replays.size();
            // this->list->reset();
        };
        addChild(text_input);
    }


    const auto gen_table_borders = [](CCNode* node, const CCPoint pos, const CCSize size, const int z) {
        auto* spr = CCSprite::createWithSpriteFrameName("GJ_table_side_001.png");
        const auto side_height = spr->getContentSize().height;
        spr->release();

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_side_001.png"))
            .setPosition(ccp(pos.x - size.width / 2.f - 9.f, pos.y))
            .setScale(1.f, size.height / side_height)
            .setZOrder(z)
        );

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_side_001.png"))
            .setPosition(ccp(pos.x + size.width / 2.f + 9.f, pos.y))
            .setScale(-1.f, size.height / side_height)
            .setZOrder(z)
        );

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_bottom_001.png"))
            .setPosition(ccp(pos.x, pos.y - size.height / 2.f))
            .setAnchorPoint(ccp(0.5f, 0.81f))
            .setZOrder(z)
        );

        node->addChild(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_table_top_001.png"))
            .setPosition(ccp(pos.x, pos.y + size.height / 2.f))
            .setAnchorPoint(ccp(0.5f, 0.21f))
            .setZOrder(z)
        );
    };

    gen_table_borders(this, win_size / 2.f, {350, 65 * 3 + 40.f}, 4);

    setKeypadEnabled(true);
    return true;
}


CCNode* ReplaysLayer::make_widget(size_t index) {
    if (index >= replays.size()) return nullptr;

    const Replay& replay = this->filter.empty() ? replays[index] : this->filtered_replays[index];

    auto* node = CCNode::create();
    node->setZOrder(1);

    const unsigned char bg_color = index % 2 == 0 ? 50 : 35;

    // background
    node->addChild(
        NodeFactory<CCLayerColor>::start(
            ccColor4B { bg_color, bg_color, bg_color, 100 },
            widget_width, widget_height
        )
        .setZOrder(-5)
        .setPosition(-widget_width / 2.f, -widget_height / 2.f)
    );

    auto menu = CCMenu::create();
    node->addChild(menu);
    menu->setPosition(0, 0);

    menu->addChild(
        NodeFactory<gd::CCMenuItemSpriteExtra>::start(
            NodeFactory<gd::ButtonSprite>::start("View", 50, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.8f)
                .setScale(0.8f).end(),
            this, menu_selector(ReplaysLayer::on_view)
        )
        .setPosition(ccp(widget_width / 2.f - 40.f, 0.f))
        .setTag(index)
    );

    // level name
    node->addChild(
        NodeFactory<CCLabelBMFont>::start(replay.level_name.c_str(), "bigFont.fnt")
        .setAnchorPoint(ccp(0, 0))
        .setPosition(ccp(-widget_width / 2.f + 60.f, 0.f))
        .with([](auto* node) { node->limitLabelWidth(180.f, 0.8f, 0.2f); })
    );

    // percent
    node->addChild(
        NodeFactory<CCLabelBMFont>::start(fmt::format("{}%", int(replay.died_at)).c_str(), "goldFont.fnt")
        .setAnchorPoint(ccp(0.f, 1.f))
        .setPosition(ccp(-widget_width / 2.f + 60.f, 0.f))
        .setScale(0.6f)
    );

    // difficulty icon
    node->addChild(
        NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("diffIcon_06_btn_001.png"))
        .setPosition(ccp(-widget_width / 2.f + 30.f, 0.f))
        .setScale(1.1f)
    );

    // timestamp
    node->addChild(
        NodeFactory<CCLabelBMFont>::start("2022-08-27 16:21", "chatFont.fnt")
        .setPosition(ccp(widget_width / 2.f - 3.f, -widget_height / 2.f + 3.f))
        .setScale(0.6f)
        .setAnchorPoint(ccp(1.f, 0.f))
        .setColor(ccc3(220, 220, 220))
    );

    return node;
}


void ReplaysLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void ReplaysLayer::on_view(CCObject* sender) {
    auto btn = cast<gd::CCMenuItemSpriteExtra*>(sender);
    const auto i = static_cast<size_t>(btn->getTag());
    auto& rs = ReplaySystem::get_instance();
    if (i >= replays.size()) return;
    auto& replay = replays[i];
    auto d = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    auto lvl = d[std::to_string(replay.level_id)];
    if (lvl && !lvl->levelNotDownloaded) {
        rs.get_replay() = replay;
        rs.toggle_playing();
        auto layer = gd::PlayLayer::create(lvl);
        layer->m_isTestMode = true; // ez safe mode
        rs.update_status_label();
        CCDirector::sharedDirector()->pushScene(NodeFactory<CCScene>::start().addChild(layer));
    }
}

void ReplaysLayer::gen_widgets() {
    // if (widgets)
    //     widgets->removeAllChildrenWithCleanup(true);
    // else {
    //     widgets = CCNode::create();
    //     widgets->setPosition(ccp(0, 0));
    //     addChild(widgets);
    // }

    // const auto win_size = CCDirector::sharedDirector()->getWinSize();
    // float y = win_size.height - 90;
    
    // for (auto& [i, replay] : enumerate(replays)) {
    //     if (i < scroll || i > scroll + 2) continue;
    //     auto widget = CCNode::create();
    //     widget->addChild(NodeFactory<extension::CCScale9Sprite>::start("GJ_square01.png").setContentSize(ccp(400, 60)));
    //     widget->setPosition(280, y);
    //     auto menu = CCMenu::create();
    //     widget->addChild(menu);
    //     menu->setPosition(0, 0);

    //     menu->addChild(
    //         NodeFactory<gd::CCMenuItemSpriteExtra>::start(
    //             gd::ButtonSprite::create("see", 50, false, "goldFont.fnt", "GJ_button_01.png", 0, 1.f), this, menu_selector(ReplaysLayer::on_view))
    //         .setPosition(152.f, 0.f)
    //         .setTag(i)
    //     );

    //     widget->addChild(
    //         NodeFactory<CCLabelBMFont>::start(fmt::format("{}\n{}%", replay.level_name, int(replay.died_at)).c_str(), "bigFont.fnt")
    //         .setPosition(-185, 0)
    //         .setScale(0.4f)
    //         .setAnchorPoint(ccp(0, 0.5f))
    //         .setAlignment(kCCTextAlignmentLeft)
    //     );

    //     widgets->addChild(widget);

    //     y -= 70.f;
    // }
}

template <class T, class U, class V>
inline T clamp(const T value, const U lower, const V upper) {
    return value > upper ? upper : value < lower ? lower : value;
}

void ReplaysLayer::on_scroll_arrow(CCObject* sender) {
    bool down = cast<CCNode*>(sender)->getTag() == 1;
    auto new_scroll = clamp(int(scroll) + (down * 2 - 1), 0, int(replays.size()) - 1);
    if (scroll != new_scroll) {
        scroll = new_scroll;
        logln("scrolling to {}", scroll);
        gen_widgets();
    }
}
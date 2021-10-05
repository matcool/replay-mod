#include "replays_layer.hpp"
#include "nodes.hpp"
#include "replay_system.hpp"

template <typename T>
struct enumerator_iterator {
    using deref_type = typename decltype(std::declval<T&>().operator*());
    T& iterator;
    size_t index;
    enumerator_iterator(T& it, size_t index) : iterator(it), index(index) {}
    std::pair<size_t, deref_type> operator*() { return { index, *iterator }; }

    auto& operator++() { 
        ++index;
        ++iterator;
        return *this;
    }

    friend bool operator==(const enumerator_iterator<T>& a, const enumerator_iterator<T>& b) { return a.iterator == b.iterator; };
    friend bool operator!=(const enumerator_iterator<T>& a, const enumerator_iterator<T>& b) { return a.iterator != b.iterator; };
};

template <typename T>
struct enumerate {
    T& parent;
    enumerate(T& parent): parent(parent) {}
    auto begin() { return enumerator_iterator(parent.begin(), 0); }
    auto end() { return enumerator_iterator(parent.end(), 0); }
};


template <typename K, typename T>
struct CCDictIterator {
public:
    CCDictIterator(CCDictElement* p) : m_ptr(p) {}
    CCDictElement* m_ptr;

    std::pair<K, T> operator*() {
        if constexpr (std::is_same<K, std::string>::value) {
            return { m_ptr->getStrKey(), reinterpret_cast<T>(m_ptr->getObject()) };
        } else {
            return { m_ptr->getIntKey(), reinterpret_cast<T>(m_ptr->getObject()) };
        }
    }

    auto& operator++() {
        m_ptr = reinterpret_cast<decltype(m_ptr)>(m_ptr->hh.next);
        return *this;
    }

    friend bool operator== (const CCDictIterator<K, T>& a, const CCDictIterator<K, T>& b) { return a.m_ptr == b.m_ptr; };
    friend bool operator!= (const CCDictIterator<K, T>& a, const CCDictIterator<K, T>& b) { return a.m_ptr != b.m_ptr; };
    bool operator!= (int b) { return m_ptr != nullptr; }
};


template <typename K, typename T>
struct AwesomeDict {
public:
    AwesomeDict(CCDictionary* dict) : m_dict(dict) {}
    CCDictionary* m_dict;
    auto begin() { return CCDictIterator<K, T>(m_dict->m_pElements); }
    // do not use this
    auto end() {
        return 0;
    }

    auto size() { return m_dict->count(); }
    T operator[](K key) { return reinterpret_cast<T>(m_dict->objectForKey(key)); }
};

bool ReplaysLayer::init() {
    if (!CCLayer::init()) return false;
    auto& rs = ReplaySystem::get_instance();
    auto saved_levels = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    for (auto& [i, replay] : enumerate(rs.temp_replays)) {
        auto widget = CCNode::create();
        widget->addChild(NodeFactory<extension::CCScale9Sprite>::start("GJ_square01.png").setContentSize(ccp(400, 60)));
        widget->setPosition(280, 35 + i * 70);
        auto menu = CCMenu::create();
        widget->addChild(menu);
        menu->setPosition(0, 0);
        menu->addChild(
            NodeFactory<gd::CCMenuItemSpriteExtra>::start(
                gd::ButtonSprite::create("see", 50, false, "goldFont.fnt", "GJ_button_01.png", 0, 1.f), this, menu_selector(ReplaysLayer::on_see))
            .setPosition(152, 0)
            .setTag(i));
        std::string level_name = "unknown";
        if (auto level = saved_levels[std::to_string(replay->level_id)]; level && !level->levelNotDownloaded) {
            level_name = level->levelName;
        }
        widget->addChild(
            NodeFactory<CCLabelBMFont>::start(level_name.c_str(), "bigFont.fnt")
            .setPosition(-185, 0)
            .setScale(0.55f)
            .setAnchorPoint(ccp(0, 0.5f)));
        addChild(widget);
    }
    setKeypadEnabled(true);
    return true;
}

void ReplaysLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void ReplaysLayer::on_see(CCObject* sender) {
    auto btn = cast<gd::CCMenuItemSpriteExtra*>(sender);
    auto i = btn->getTag();
    auto& rs = ReplaySystem::get_instance();
    if (i < 0 || i >= rs.temp_replays.size()) return;
    auto replay = rs.temp_replays[i];
    auto d = AwesomeDict<std::string, gd::GJGameLevel*>(gd::GameLevelManager::sharedState()->m_onlineLevels);
    auto lvl = d[std::to_string(replay->level_id)];
    if (lvl && !lvl->levelNotDownloaded) {
        rs.get_replay() = *replay;
        rs.toggle_playing();
        auto layer = gd::PlayLayer::create(lvl);
        layer->m_isTestMode = true; // ez safe mode
        rs.update_status_label();
        CCDirector::sharedDirector()->pushScene(NodeFactory<CCScene>::start().addChild(layer));
    }
}
#pragma once
#include "includes.h"
#include <gd.h>
#include <functional>
#include <stdexcept>

#define GEN_CREATE(class_name) template <typename... Args> \
static auto create(Args... args) { \
    auto node = new class_name; \
    if (node && node->init(args...)) \
        node->autorelease(); \
    else \
        CC_SAFE_DELETE(node); \
    return node; \
}

class TextInputNode : public CCNode, public gd::TextInputDelegate {
public:
    gd::CCTextInputNode* input_node;
    cocos2d::extension::CCScale9Sprite* background;
    std::function<void(TextInputNode*)> callback = [](auto){};

    GEN_CREATE(TextInputNode)

    bool init(CCSize size, float scale = 1.f, const std::string& placeholder = "", const std::string& font = "bigFont.fnt", bool anchor_left = false) {
        if (!CCNode::init()) return false;

        input_node = gd::CCTextInputNode::create(placeholder.c_str(), this, font.c_str(), size.width, size.height);
        input_node->setDelegate(this);
        addChild(input_node);

        if (anchor_left) {
            // um nice hardcoded 10.f
            input_node->setPosition(-size.width / 2.f + 10.f, 0.f);
            input_node->getPlaceholderLabel()->setAnchorPoint(ccp(0.f, 0.5f));
            input_node->m_pTextField->setAnchorPoint(ccp(0.f, 0.5f));
        }

        background = extension::CCScale9Sprite::create("square02_small.png");
        background->setContentSize(size * scale);
        background->setScale(1.f / scale);
        background->setOpacity(100);
        background->setZOrder(-1);
        addChild(background);

        return true;
    }



    virtual void textChanged(gd::CCTextInputNode*) { callback(this); }

    void set_value(const std::string& value) { input_node->setString(value.c_str()); }
    std::string get_value() { return input_node->getString(); }
};

class NumberInputNode : public TextInputNode {
public:
    std::function<void(NumberInputNode*)> callback = [](auto){};
    
    GEN_CREATE(NumberInputNode)

    bool init(CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt") {
        if (!TextInputNode::init(size, scale, font)) return false;
        input_node->setAllowedChars("0123456789");
        return true;
    }
    virtual void textChanged(gd::CCTextInputNode*) { callback(this); }

    void set_value(int value) { input_node->setString(std::to_string(value).c_str()); }
    int get_value() {
        try {
            return std::stoi(input_node->getString());
        } catch (const std::invalid_argument&) {
            return -1;
        }
    }
};

template <typename T>
class NodeFactory {
public:
    static auto& start(T* node) { return *reinterpret_cast<NodeFactory*>(node); }
    
    template <typename... Args>
    static auto& start(Args... args) { return *reinterpret_cast<NodeFactory*>(T::create(args...)); }
    
    T* end() { return reinterpret_cast<T*>(this); }

    operator T*() { return end(); }

    inline auto& setPosition(const CCPoint p) { end()->setPosition(p); return *this; }
    inline auto& setPosition(float x, float y) { return this->setPosition(ccp(x, y)); }
    inline auto& setScale(float x, float y) { return this->setScaleX(x).setScaleY(y); }
    inline auto& setScale(float x) { end()->setScale(x); return *this; }

    #define _gen_func(name) template <typename... Args> \
    inline auto& name(Args... args) { \
        end()->name(args...); \
        return *this; \
    }

    // _gen_func(setPosition)
    // _gen_func(setScale)
    _gen_func(setScaleX)
    _gen_func(setScaleY)
    _gen_func(setContentSize)
    _gen_func(setOpacity)
    _gen_func(setZOrder)
    _gen_func(setAnchorPoint)
    _gen_func(addChild)
    _gen_func(setTag)
    _gen_func(setAlignment)
    _gen_func(setColor)
    _gen_func(limitLabelWidth)
    _gen_func(setUserObject)

    #undef _gen_func

    template <class Func>
    auto& with(const Func& function) {
        function(reinterpret_cast<T*>(this));
        return *this;
    }
};

template <class T>
auto& make_factory(T* node) {
    return NodeFactory<T>::start(node);
}

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

inline CCRect make_rect(const CCPoint& origin, const CCSize& size) {
    return CCRect(origin.x, origin.y, size.width, size.height);
}

// gets mouse coordinates in cocos2d coords
inline CCPoint get_mouse_pos() {
    auto* director = CCDirector::sharedDirector();
    auto* view = director->getOpenGLView();
    const auto win_size = director->getWinSize();
    auto mouse_pos = view->getMousePosition() / view->getFrameSize() * win_size;
    mouse_pos.y = win_size.height - mouse_pos.y;
    return mouse_pos;
}

inline void set_menu_size(CCMenu* menu) {
    menu->setContentSize(static_cast<CCNode*>(menu->getChildren()->objectAtIndex(0))->getScaledContentSize());
}
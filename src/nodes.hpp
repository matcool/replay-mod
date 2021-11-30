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

    bool init(CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt") {
        if (!CCNode::init()) return false;

        input_node = gd::CCTextInputNode::create("", this, font.c_str(), size.width, size.height);
        input_node->setDelegate(this);
        addChild(input_node);

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

    #define _gen_func(name) template <typename... Args> \
    inline auto& name(Args... args) { \
        reinterpret_cast<T*>(this)->name(args...); \
        return *this; \
    }

    _gen_func(setPosition)
    _gen_func(setScale)
    _gen_func(setContentSize)
    _gen_func(setOpacity)
    _gen_func(setZOrder)
    _gen_func(setAnchorPoint)
    _gen_func(addChild)
    _gen_func(setTag)
    _gen_func(setAlignment)

    #undef _gen_func
};

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
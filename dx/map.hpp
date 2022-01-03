#pragma once

/// purposely plain map for the time being
/// intuitive design is fifo-style behaviour, and thats essentially all thats here now
template <typename K, typename V>
class  map;
struct var;

template <typename K, typename V>
struct pair {
    K key;
    V value;
};

template <typename K, typename V>
class map:io {
public:
    std::vector<pair<K,V>> pairs;
    static typename std::vector<pair<K,V>>::iterator iterator;
    
    map(nullptr_t n = null) { }
    
    map(std::initializer_list<pair<K,V>> p) {
        pairs.reserve(p.size());
        for (auto &i: p)
            pairs.push_back(i);
    }
    
    map(size_t reserve) {
        pairs.reserve(reserve);
    }
    
    map(const map<K,V> &p) {
        pairs.reserve(p.size());
        for (auto &i: p.pairs)
            pairs.push_back(i);
    }
    
    inline int index(K k) const {
        int index = 0;
        for (auto &i: pairs) {
            if (k == i.key)
                return index;
            index++;
        }
        return -1;
    }
    
    inline size_t count(K k) const {
        return index(k) >= 0 ? 1 : 0;
    }
    
    inline V &operator[](K k) {
        for (auto &i: pairs)
            if (k == i.key)
                return i.value;
        pairs.push_back(pair <K,V> { k });
        return pairs.back().value;
    }
    
    inline V &back()                { return pairs.back();     }
    inline V &front()               { return pairs.front();    }
    inline V *data()                { return pairs.data();     }
    inline typeof(iterator) begin() { return pairs.begin();    }
    inline typeof(iterator) end()   { return pairs.end();      }
    inline size_t size() const      { return pairs.size();     }
    inline size_t capacity() const  { return pairs.capacity(); }
    
    inline bool erase(K k) {
        size_t index = 0;
        for (auto &i: pairs) {
            if (k == i.key) {
                pairs.erase(pairs.begin() + index);
                return true;
            }
            index++;
        }
        return false;
    }
    
    inline bool operator==(map<K,V> &b) {
        if (size() != b.size())
            return false;
        for (auto &[k,v]: b)
            if ((*this)[k] != v)
                return false;
        return true;
    }
    
    inline bool operator!=(map<K,V> &b) {
        return !operator==(b);
    }
};

template<typename>
struct is_map                    : std::false_type {};

template<typename K, typename V>
struct is_map<map<K,V>>          : std::true_type  {};

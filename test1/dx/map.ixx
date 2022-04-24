export module map;

import std.core;
import io;

export {

/// purposely plain fmap for the time being, simple fifo; using proven techniques such as no-hash-compute.

template <typename K, typename V>
struct pair {
    K key;
    V value;
};

template <typename K, typename V>
struct map:io {
    typedef std::vector<pair<K,V>>  vpairs;
    std::shared_ptr<vpairs>          pairs;
    static typename vpairs::iterator iterator;
    ///
    vpairs &realloc(size_t sz)              {
        pairs = std::shared_ptr<vpairs>(new vpairs());
        if (sz)
            pairs->reserve(sz);
        return *pairs;
    }
    
    /// bit weird here.. i am just looking to narrow words map::base64() is a base64 validation/value map
    static map<int, int> base64() {
        auto m = map<int, int>();
        for (int i = 0; i < 26; i++)
            m['A' + i] = i;
        for (int i = 0; i < 26; i++)
            m['a' + i] = 26 + i;
        for (int i = 0; i < 10; i++)
            m['0' + i] = 26 * 2 + i;
        m['+'] = 26 * 2 + 10 + 0;
        m['/'] = 26 * 2 + 10 + 1;
        return m;
    }

    map(std::nullptr_t n = null)            { realloc(0); }
    map(size_t         sz)                  { realloc(sz); }
    void reserve(size_t sz)                 { pairs->reserve(sz); }
    void clear(size_t sz = 0)               { pairs->clear(); if (sz) pairs->reserve(sz); }
    map(std::initializer_list<pair<K,V>> p) {
        realloc(p.size());
        for (auto &i: p) pairs->push_back(i);
    }

    map(const map<K,V> &p) {
        realloc(p.size());
        for (auto &i: *p.pairs)
            pairs->push_back(i);
    }
    
    inline int index(K k) const {
        int index = 0;
        for (auto &i: *pairs) {
            if (k == i.key)
                return index;
            index++;
        }
        return -1;
    }
    
    inline size_t count(K k) const { return index(k) >= 0 ? 1 : 0; }
    inline V &operator[](const char *k) {
        K kv = k;
        for (auto &i: *pairs)
            if (kv == i.key)
                return i.value;
        pairs->push_back(pair <K,V> { kv });
        return pairs->back().value;
    }
    inline V &operator[](K k) {
        for (auto &i: *pairs)
            if (k == i.key)
                return i.value;
        pairs->push_back(pair <K,V> { k });
        return pairs->back().value;
    }
    
    inline V         &back()        { return pairs->back();     }
    inline V        &front()        { return pairs->front();    }
    inline V         *data()  const { return pairs->data();     }
    inline vpairs::iterator begin() { return pairs->begin();    } /// change all iterator instances
    inline vpairs::iterator   end() { return pairs->end();      }
    inline size_t     size()  const { return pairs->size();     }
    inline size_t capacity()  const { return pairs->capacity(); }
    inline bool      erase(K k) {
        size_t index = 0;
        for (auto &i: *pairs) {
            if (k == i.key) {
                pairs->erase(pairs->begin() + index);
                return true;
            }
            index++;
        }
        return false;
    }

    inline bool operator!=(map<K,V> &b) { return !operator==(b);    }
    inline operator bool()              { return pairs->size() > 0; }
    inline bool operator==(map<K,V> &b) {
        if (size() != b.size())
            return false;
        for (auto &[k,v]: b)
            if ((*this)[k] != v)
                return false;
        return true;
    }
};

template<typename>
struct is_map                 : std::false_type {};

template<typename K, typename V>
struct is_map<map<K,V>>       : std::true_type  {};
///
}
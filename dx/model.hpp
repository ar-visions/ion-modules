#pragma once

#include <dx/io.hpp>
#include <dx/map.hpp>
#include <dx/string.hpp>
#include <dx/array.hpp>

typedef map<str, var> Schema;
typedef map<str, var> SMap; /// make this Map if its used.
typedef array<var>     Table;
typedef map<str, var>  ModelMap;

inline var ModelDef(str name, Schema schema) {
    var m   = schema;
        m.s = name;
    return m;
}

/// re-integrate!
struct Remote {
        int64_t value;
            str remote;
    Remote(int64_t value)                  : value(value)                 { }
    Remote(std::nullptr_t n = nullptr)          : value(-1)                    { }
    Remote(str remote, int64_t value = -1) : value(value), remote(remote) { }
    operator int64_t() { return value; }
    operator var() {
        if (remote) {
            var m = { Type::Map, Type::Undefined }; /// invalid state used as trigger.
                m.s = string(remote);
            m["resolves"] = var(remote);
            return m;
        }
        return var(value);
    }
};
///
struct Ident   : Remote {
       Ident() : Remote(null) { }
};

/*
template <typename T>
struct DLNode {
    DLNode  *next = null,
            *prev = null;
    T    *payload = null;
    uint8_t  data[1];
    ///
    static T *alloc() {
        DLNode<T> *b = calloc(1, sizeof(DLNode<T>) + sizeof(T));
         b->payload  = b->data;
        *b->payload  = T();
        return b;
    }
};

template <typename T>
struct DList {
    DLNode<T> *first = null;
    DLNode<T> *last = null;
};

/// the smart pointer with baked in array and a bit of indirection.
template <typename T>
struct list:ptr<DList<T>> {
    DList<T> *m;
    /// -------------------
    list() { alloc(m); }
    T *push() {
        DLNode<T> *n = DLNode<T>::alloc();
        ///
        if (m->last)
            m->last->next = n;
        ///
        n->prev = m->last;
        m->last = n;
        ///
        if (!m->first)
            m->first = n;
        ///
        return T();
    }
    void pop() {
        // release taht memory
    }
};
*/
